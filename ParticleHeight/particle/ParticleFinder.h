#pragma once
#include "image/ImageProcessor.h"
#include "Particle.h"
#include "ray/OpticalScene.h"
#include "ray/OpticalLayer.h"
#include "ray/OpticalPattern.h"
#include "ray/OpticalSphere.h"
#include "TransformSingle.h"
#include "TransformMultiple.h"
#include "util/Settings.h"
#include "nlopt.hpp"
#include <list>

namespace ph
{
	class ParticleFinder
	{
	public:
		ParticleFinder(const ImageProcessor* imp, const Settings* s, bool verbose = true) : m_pRefProcessor(imp), m_pSettings(s), m_bVerbose(verbose) {};
		ParticleFinder() : m_pRefProcessor(nullptr), m_pSettings(nullptr), m_bVerbose(true) {};
		~ParticleFinder() {};
	public:
		std::list<Particle> findParticles(cv::Mat matParticle, bool bUseHough = false);
	private:
		bool m_findHeightSingle(Particle*, cv::Mat matParticle, double& dConfidence, unsigned& nEvals);
		bool m_findHeightGroup(std::vector<Particle*>, cv::Mat matParticle, double& dConfidence, unsigned& nEvals);
	private:
		const ImageProcessor* m_pRefProcessor;
		const Settings* m_pSettings;
	private:
		bool m_bVerbose;
	};

	struct dataCorrelate
	{
		const ImageProcessor* refProcessor;
		const Settings* settings;
		cv::Mat* matParticle;
		OpticalScene* scene;
		unsigned nEvals;
	};

	double correlateSingleParticle(unsigned n, const double* pos, double* grad, void* data);

	double correlateGroupParticle(unsigned n, const double* pos, double* grad, void* data);
}
