#include "ParticleFinder.h"
#include <math.h>
#include <list>
#include <iostream>
#include <chrono>

using namespace ph;

std::list<Particle> ph::ParticleFinder::findParticles(cv::Mat matParticle, bool bUseHough)
{
	// matParticle should already be aligned to the reference pattern image
	if (m_bVerbose) std::cout << "finding particles...";

	// Preprocess a copy of the image
	cv::Mat matParticleCopy = matParticle.clone();
	m_pRefProcessor->subtractBackground(matParticleCopy);
	m_pRefProcessor->morphClose(matParticleCopy);
	m_pRefProcessor->morphOpen(matParticleCopy);
	
	// Use Hough transform or EDT to initially find circles
	std::vector<cv::Vec3f> vecCircles;
	if (bUseHough)
		vecCircles = m_pRefProcessor->findCirclesHough(matParticleCopy);
	else
		vecCircles = m_pRefProcessor->findCirclesEDT(matParticleCopy);

	// The preprocessed image is no longer needed - release it
	matParticleCopy.release();

	// begin timing particle finding
	auto startTime = std::chrono::high_resolution_clock::now();

	// create a list of particles from the vector of circles
	std::list<Particle> listParticles(vecCircles.size());
	int i = 0;
	for (auto it = listParticles.begin(); it != listParticles.end(); ++it, ++i)
		it->setPosition(vecCircles[i][0], vecCircles[i][1]);

	// update each particle's neighbors
	for (auto p1 = listParticles.begin(); p1 != listParticles.end(); ++p1)
		for (auto p2 = std::next(p1); p2 != listParticles.end(); ++p2)
			if (p1->isOverlapping(*p2))
			{
				p1->addNeighbor(&(*p2));
				p2->addNeighbor(&(*p1));
			}

	// find height of every overlapping group of particles (including single particles)
	int nSingleParticles = 0, nGroups = 0;
	unsigned nTotalSingleEvals = 0, nTotalGroupEvals = 0, nEvals;
	for (auto& p : listParticles)
		if (!p.isHeightKnown())
			if (p.hasNeighbors())
			{
				// find the height of the overlapping particle group
				nGroups++;
				std::vector<Particle*> vecpGroup;
				p.getOverlapGroup(vecpGroup);
				double dConfidence;
				if (m_findHeightGroup(vecpGroup, matParticle, dConfidence, nEvals))
				{
					nTotalGroupEvals += nEvals;
					for (auto p : vecpGroup)
					{
						p->setHeightKnown(true);

						// update the confidence for this particle by constructing a scene with its nearest neighbors
						OpticalScene scene(m_pSettings->fEtaLiquid);
						scene.addMedium(std::make_shared<OpticalPattern>(vf3(0, 0, 0), vf3(0, 0, 1)));  // pattern
						scene.addMedium(std::make_shared<OpticalLayer>(vf3(0, 0, 0), vf3(0, 0, m_pSettings->fChannelWallThickness), vf3(0, 0, 1), m_pSettings->fEtaGlass));  // bottom channel wall
						scene.addMedium(std::make_shared<OpticalSphere>(p->getPositionReal(), p->getRadiusReal(), m_pSettings->fEtaParticle));  // add the particle
						for (auto n : p->getNeighbors())
							scene.addMedium(std::make_shared<OpticalSphere>(n->getPositionReal(), n->getRadiusReal(), m_pSettings->fEtaParticle));  // add each neighbor

						float posX, posY;
						p->getPositionPx(posX, posY);
						cv::Rect rectRegion((int)posX - (m_pSettings->nDICRegionSize >> 1), (int)posY - (m_pSettings->nDICRegionSize >> 1), m_pSettings->nDICRegionSize, m_pSettings->nDICRegionSize);
						p->setConfidence((float)(m_pRefProcessor->correlateTransform(TransformMultiple(p, m_pSettings, &scene), rectRegion, matParticle)));
					}
				}
			}
			else
			{
				// find the height of the single particle
				nSingleParticles++;
				double dConfidence;
				if (m_findHeightSingle(&p, matParticle, dConfidence, nEvals))
				{
					nTotalSingleEvals += nEvals;
					p.setHeightKnown(true);
					p.setConfidence((float)dConfidence);
				}
			}

	auto endTime = std::chrono::high_resolution_clock::now();

	// print results
	if (m_bVerbose)
		std::cout << "\rfound " << listParticles.size() << " particles (" << nSingleParticles
			<< " individual with avg. " << ((nSingleParticles == 0) ? 0 : nTotalSingleEvals/nSingleParticles) 
			<< " evals, " << nGroups << " groups with avg. " << ((nGroups == 0) ? 0 : nTotalGroupEvals/nGroups) 
			<< " evals) in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
			<< " ms" << std::endl;

	return listParticles;
}

bool ph::ParticleFinder::m_findHeightSingle(Particle* pParticle, cv::Mat matParticle, double& dConfidence, unsigned& nEvals)
{
	
	bool bFoundParticleHeight = false;
	
	// create data object to give to objective function, doesn't have a scene since analytical model used here
	dataCorrelate data{ m_pRefProcessor, m_pSettings, &matParticle, nullptr, 0 };

	nlopt::opt optimizer(nlopt::algorithm::LN_NELDERMEAD, 3);
	optimizer.set_max_objective(correlateSingleParticle, &data);
	std::vector<double> vecInitialStep(3, m_pSettings->fInitStepSingle);
	optimizer.set_initial_step(vecInitialStep);
	optimizer.set_xtol_abs(m_pSettings->fXtolAbsSingle);
	std::vector<double> position = { pParticle->getPositionReal().x, pParticle->getPositionReal().y, pParticle->getPositionReal().z };

	try
	{
		optimizer.optimize(position, dConfidence);
		pParticle->setPosition(vf3(position[0], position[1], position[2]));
		bFoundParticleHeight = true;
		nEvals = data.nEvals;
	}
	catch (std::exception& e)
	{
		std::cout << "NLopt failed: " << e.what() << std::endl;
	}

	return bFoundParticleHeight;
}

bool ph::ParticleFinder::m_findHeightGroup(std::vector<Particle*> vecpParticle, cv::Mat matParticle, double& dConfidence, unsigned& nEvals)
{
	
	bool bFoundParticleHeights = false;

	// for an initial guess, run the single particle height finding routine on each particle
	double dTemp;
	unsigned nTemp;
	for (auto p : vecpParticle)
		m_findHeightSingle(p, matParticle, dTemp, nTemp);

	// create the optical scene representing this group of particles
	// add the particles first, followed by the pattern, and then the optical layer
	OpticalScene scene(m_pSettings->fEtaLiquid);
	for (auto p : vecpParticle)
		scene.addMedium(std::make_shared<OpticalSphere>(p->getPositionReal(), p->getRadiusReal(), m_pSettings->fEtaParticle));  // add each particle
	scene.addMedium(std::make_shared<OpticalPattern>(vf3(0, 0, 0), vf3(0, 0, 1)));  // pattern
	scene.addMedium(std::make_shared<OpticalLayer>(vf3(0, 0, 0), vf3(0, 0, m_pSettings->fChannelWallThickness), vf3(0, 0, 1), m_pSettings->fEtaGlass));  // bottom channel wall
	
	// data structure for passing objective function state to optimizer
	dataCorrelate data{ m_pRefProcessor, m_pSettings, &matParticle, &scene, 0 };

	// set up NLopt optimizer object
	nlopt::opt optimizer(nlopt::algorithm::LN_NELDERMEAD, 3 * vecpParticle.size());
	optimizer.set_max_objective(correlateGroupParticle, &data);
	std::vector<double> vecInitialStep(3 * vecpParticle.size(), m_pSettings->fInitStepGroup);
	optimizer.set_initial_step(vecInitialStep);
	optimizer.set_xtol_abs(m_pSettings->fXtolAbsGroup);

	// fill initial guess vector with original particle positions
	std::vector<double> position(3 * vecpParticle.size());
	for (size_t i = 0; i < vecpParticle.size(); ++i)
	{
		position[3 * i + 0] = vecpParticle[i]->getPositionReal().x;
		position[3 * i + 1] = vecpParticle[i]->getPositionReal().y;
		position[3 * i + 2] = vecpParticle[i]->getPositionReal().z;
	}

	try
	{
		// perform optimization
		optimizer.optimize(position, dConfidence);
		dConfidence /= vecpParticle.size();
		nEvals = data.nEvals;

		// update particle positions
		for (size_t i = 0; i < vecpParticle.size(); ++i)
			vecpParticle[i]->setPosition(vf3(position[3 * i + 0], position[3 * i + 1], position[3 * i + 2]));

		bFoundParticleHeights = true;
	}
	catch (std::exception& e)
	{
		std::cout << "NLopt failed: " << e.what() << std::endl;
	}

	return bFoundParticleHeights;
}

double ph::correlateSingleParticle(unsigned n, const double* pos, double* grad, void* data)
{
	// create a temporary particle from the given position array
	Particle p;
	p.setPosition(vf3(pos[0], pos[1], pos[2]));
	float posX, posY;
	p.getPositionPx(posX, posY);

	// extract data from the correlation data object
	dataCorrelate* d = (dataCorrelate*)data;
	const ImageProcessor* processor = d->refProcessor;
	const Settings* settings = d->settings;
	cv::Mat im = *(d->matParticle);
	d->nEvals++; // increment the number of function evaluations

	// perform the correlation
	cv::Rect rectRegion((int)posX - (settings->nDICRegionSize >> 1), (int)posY - (settings->nDICRegionSize >> 1), settings->nDICRegionSize, settings->nDICRegionSize);
	double correlation = processor->correlateTransform(TransformSingle(&p, settings), rectRegion, im);

	// penalize overlap with the channel walls
	if (p.getPositionReal().z < settings->fChannelWallThickness + p.getRadiusReal())
		correlation -= 0.01 * settings->nOverlapPenalty * ((double)p.getPositionReal().z - settings->fChannelWallThickness - p.getRadiusReal())
					   * ((double)p.getPositionReal().z - settings->fChannelWallThickness - p.getRadiusReal());
	if (p.getPositionReal().z > settings->fChannelWallThickness + settings->fChannelHeight - p.getRadiusReal())
		correlation -= 0.01 * settings->nOverlapPenalty * ((double)p.getPositionReal().z - settings->fChannelWallThickness - settings->fChannelHeight + p.getRadiusReal())
					   * ((double)p.getPositionReal().z - settings->fChannelWallThickness - settings->fChannelHeight + p.getRadiusReal());

	return correlation;
}

double ph::correlateGroupParticle(unsigned n, const double* pos, double* grad, void* data)
{
	// extract needed information from the correlation data object
	dataCorrelate* d = (dataCorrelate*)data;
	const ImageProcessor* processor = d->refProcessor;
	const Settings* settings = d->settings;
	cv::Mat im = *(d->matParticle);
	OpticalScene* scene = d->scene;
	d->nEvals++; // increment the number of function evaluations

	// create vector of particles from the given position array and update the optical scene
	n /= 3;
	std::vector<Particle> vecP(n);
	std::vector<std::shared_ptr<OpticalMedium>> media = scene->getMedia();
	for (unsigned i = 0; i < n; ++i)
	{
		vecP[i].setPosition(vf3(pos[3 * i + 0], pos[3 * i + 1], pos[3 * i + 2]));
		OpticalSphere* s = (OpticalSphere*)media[i].get();
		s->setPosition(vf3(pos[3 * i + 0], pos[3 * i + 1], pos[3 * i + 2]));
	}

	// accumulate the correlation for each particle in the scene
	double correlation = 0;
	for (auto p = vecP.begin(); p != vecP.end(); ++p)
	{
		float posX, posY;
		p->getPositionPx(posX, posY);
		cv::Rect rectRegion((int)posX - (settings->nDICRegionSize >> 1), (int)posY - (settings->nDICRegionSize >> 1), settings->nDICRegionSize, settings->nDICRegionSize);
		correlation += processor->correlateTransform(TransformMultiple(&(*p), settings, scene), rectRegion, im);

		// quadratic penalty for overlap between particles or with the walls
		if (p->getPositionReal().z < settings->fChannelWallThickness + p->getRadiusReal())
			correlation -= 0.01 * settings->nOverlapPenalty * ((double)p->getPositionReal().z - settings->fChannelWallThickness - p->getRadiusReal())
								     * ((double)p->getPositionReal().z - settings->fChannelWallThickness - p->getRadiusReal());
		if (p->getPositionReal().z > settings->fChannelWallThickness + settings->fChannelHeight - p->getRadiusReal())
			correlation -= 0.01 * settings->nOverlapPenalty * ((double)p->getPositionReal().z - settings->fChannelWallThickness - settings->fChannelHeight + p->getRadiusReal())
								     * ((double)p->getPositionReal().z - settings->fChannelWallThickness - settings->fChannelHeight + p->getRadiusReal());
		for (auto n = std::next(p); n != vecP.end(); ++n)  // check overlap with neighbors
		{
			float centerDist = p->getCenterDist(*n);
			if (centerDist < p->getRadiusReal() + n->getRadiusReal())
				correlation -= 0.01 * settings->nOverlapPenalty * ((double)centerDist - p->getRadiusReal() - n->getRadiusReal())
									     * ((double)centerDist - p->getRadiusReal() - n->getRadiusReal());
		}
	}

	return correlation;
}