#include "ImageProcessor.h"
#include <opencv2/highgui.hpp>  
#include <iostream>
#include <queue>

using namespace ph;

double ph::ImageProcessor::alignToRef(cv::Mat matParticle) const
{
	// matrix to store transformation
	cv::Mat matWarp = cv::Mat::eye(2, 3, CV_32F);

	// find transformation
	double cc = cv::findTransformECC(m_matRef, matParticle, matWarp, cv::MOTION_AFFINE,
		cv::TermCriteria((cv::TermCriteria::COUNT)+(cv::TermCriteria::EPS), 50, 0.001), cv::noArray(), 5 /*gaussFiltSize*/);

	// apply transformation to the current frame
	cv::warpAffine(matParticle, matParticle, matWarp, matParticle.size(), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP);

	return cc;
}

void ph::ImageProcessor::subtractBackground(cv::Mat matParticle) const
{
	// perform background subtraction with the reference frame as the background
	// create the background subtractor object
	cv::Ptr<cv::BackgroundSubtractorMOG2> pBackSub =
		cv::createBackgroundSubtractorMOG2(1 /*history*/, m_pSettings->nBackgroundThreshold /*varThreshold*/, false /*detectShadows*/);

	// give it the reference image
	cv::Mat matTemp;
	pBackSub->apply(m_matRef, matTemp, 0.0 /*learning rate*/);

	// apply it to the current frame
	pBackSub->apply(matParticle, matParticle, 0.0 /*learning rate*/);
}

void ph::ImageProcessor::morphOpen(cv::Mat matParticle) const
{
	// remove the noise remaining after background subtraction using morphological opening
	int nOpenSize = m_pSettings->nOpenSize;
	int nOpenIter = m_pSettings->nOpenIter;

	cv::Mat matKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * nOpenSize + 1, 2 * nOpenSize + 1), cv::Point(nOpenSize, nOpenSize));
	cv::morphologyEx(matParticle, matParticle, cv::MORPH_OPEN, matKernel, cv::Point(-1, -1), nOpenIter);
}

void ph::ImageProcessor::morphClose(cv::Mat matParticle) const
{
	//perform morphological closing to refine the particles
	int nCloseSize = m_pSettings->nCloseSize;
	int nCloseIter = m_pSettings->nCloseIter;

	cv::Mat matElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * nCloseSize + 1, 2 * nCloseSize + 1), cv::Point(nCloseSize, nCloseSize));
	cv::dilate(matParticle, matParticle, matElement /*kernel*/, cv::Point(-1, -1), nCloseIter /*iterations*/);
	cv::erode(matParticle, matParticle, matElement /*kernel*/, cv::Point(-1, -1), nCloseIter /*iterations*/);
}

cv::Mat ph::ImageProcessor::morphReconstruct(cv::Mat matMarker, cv::Mat matMask) const
{
	// perform reconstruction in the marker from the mask (floating point single channel images)
	// implements fast hybrid grayscale reconstruction from Vincent 1993

	cv::Mat matDst = matMarker.clone();
	float* p_marker = (float*)matDst.data;
	float* p_mask = (float*)matMask.data;

	int ng_plus[4], ng_minus[4], ng[8];

	// scan in raster order, omitting the image borders to avoid array bounds issues
	int w = matMarker.cols;
	int n = (matMarker.cols - 2) * (matMarker.rows - 2);
	int x, y, i;
	for (int k = 0; k < n; k++)
	{
		// get out our index in the full image
		y = k / (w - 2) + 1;
		x = k % (w - 2) + 1;
		i = y * w + x;

		// calculate indices of the neighborhood
		ng_plus[0] = i - 1;
		ng_plus[1] = i - w - 1;
		ng_plus[2] = i - w;
		ng_plus[3] = i - w + 1;

		// get the max marker value in the ng_plus neighbors
		float marker_max = p_marker[i];
		for (int j = 0; j < 4; j++)
			if (p_marker[ng_plus[j]] > marker_max)
				marker_max = p_marker[ng_plus[j]];
		p_marker[i] = std::min(marker_max, p_mask[i]);
	}

	// scan in anti-raster order
	std::queue<int> q;
	for (int k = n - 1; k >= 0; k--)
	{
		// get out our index in the full image
		y = k / (w - 2) + 1;
		x = k % (w - 2) + 1;
		i = y * w + x;

		// calculate indices of the neigborhood
		ng_minus[0] = i + 1;
		ng_minus[1] = i + w + 1;
		ng_minus[2] = i + w;
		ng_minus[3] = i + w - 1;

		// get the max value in the ng_minus neighborhood
		float marker_max = p_marker[i];
		for (int j = 0; j < 4; j++)
			if (p_marker[ng_minus[j]] > marker_max)
				marker_max = p_marker[ng_minus[j]];
		p_marker[i] = std::min(marker_max, p_mask[i]);

		// add the current pixel to the queue if necessary
		for (int j = 0; j < 4; j++)
			if (p_marker[ng_minus[j]] < p_marker[i] && p_marker[ng_minus[j]] < p_mask[ng_minus[j]])
			{
				q.push(i);
				break;
			}
				
	}

	// propagation step
	while (!q.empty())
	{
		// get first thing in the queue
		int i = q.front();
		q.pop();

		// calculate indices of neighbors
		ng[0] = i - 1;
		ng[1] = i - w - 1;
		ng[2] = i - w;
		ng[3] = i - w + 1;
		ng[4] = i + 1;
		ng[5] = i + w + 1;
		ng[6] = i + w;
		ng[7] = i + w - 1;

		for (int j = 0; j < 8; j++)
			if (p_marker[ng[j]] < p_marker[i] && p_mask[ng[j]] != p_marker[ng[j]])
			{
				p_marker[ng[j]] = std::min(p_marker[i], p_mask[ng[j]]);
				q.push(ng[j]);
			}
	}

	return matDst;
}

std::vector<cv::Vec3f> ph::ImageProcessor::findCirclesHough(cv::Mat matParticle) const
{
	// find the particles with Hough circle transform
	std::vector<cv::Vec3f> vecCircles;
	cv::HoughCircles(matParticle, vecCircles, cv::HOUGH_GRADIENT, 1.5, m_pSettings->nCircleMinDist /*minDist*/, 
		m_pSettings->nCircleParam1 /*param1*/, m_pSettings->nCircleThreshold /*threshold*/, 
		m_pSettings->nCircleSize - 0.5 * m_pSettings->nCircleSizeRange /*minRadius*/, 
		m_pSettings->nCircleSize + 0.5 * m_pSettings->nCircleSizeRange /*maxRadius*/);

	// eliminate any circles which contain black pixels as these are not real particles
	// also eliminate any partial circles at the edges of the image as these won't work with the later height finding
	float fCircleIntensity = m_pSettings->nCircleIntensity;
	vecCircles.erase(std::remove_if(vecCircles.begin(), vecCircles.end(),
		[&](cv::Vec3f c)
		{
			if (c[0] < c[2] || c[1] < c[2] || c[0] + c[2] + 1 > m_matRef.cols || c[1] + c[2] + 1 > m_matRef.rows)
				return true;

			cv::Rect rectRoi = cv::Rect(c[0] - c[2], c[1] - c[2], 2 * c[2] + 1, 2 * c[2] + 1);
			cv::Mat roi = matParticle(rectRoi);
			cv::Mat1b mask = cv::Mat::zeros(roi.rows, roi.cols, 0);
			cv::circle(mask, cv::Point(c[2], c[2]), c[2], cv::Scalar(255, 255, 255), -1);
			float intensity = cv::mean(roi, mask)[0];
			return intensity < fCircleIntensity;
		}), vecCircles.end());

	return vecCircles;
}

std::vector<cv::Vec3f> ph::ImageProcessor::findCirclesEDT(cv::Mat matParticle) const
{
	// use watershed segmentation to determine the 2d locations of each particle
	// matParticle should be binarized with foreground (particles) white
	std::vector<cv::Vec3f> vecCircles;

	// pad the image with zeros on all sides and then perform distance transform
	cv::Mat matParticlePadded, matDist;
	cv::copyMakeBorder(matParticle, matParticlePadded, 1, 1, 1, 1, cv::BORDER_CONSTANT, 0);
	cv::distanceTransform(matParticlePadded, matDist, cv::DIST_L2, 3 /*mask size*/);
	cv::normalize(matDist, matDist, 0, 1.0, cv::NORM_MINMAX);  // normalize the values to range 0-1.0

	// perform the h-maxima transform to get rid of shallow minima
	cv::Mat matHMax = morphReconstruct(cv::max(0, matDist - 0.0001 * m_pSettings->nHMaxParam), matDist);

	// find the regional maxima using the reconstruction operation
	cv::Mat matMaxima = matHMax - morphReconstruct(cv::max(0, matHMax - 0.001), matHMax);
	cv::threshold(matMaxima, matMaxima, 0, 255, cv::THRESH_BINARY);
	matMaxima.convertTo(matMaxima, CV_8U);

	// find the centroids of connected components
	cv::Mat matCentroids, matLabels, matStats;
	cv::connectedComponentsWithStats(matMaxima, matLabels, matStats, matCentroids);

	// store the circle positions
	for (int i = 0; i < matCentroids.rows; i++)
	{
		// translate the particle position back into matParticle space (it was padded before)
		cv::Vec3f c(matCentroids.at<double>(i, 0) - 1, matCentroids.at<double>(i, 1) - 1, m_pSettings->fParticleRadiusPx + 1);
		vecCircles.push_back(c);
	}

	// erase any partial circles at the edges of the image, or ones that are not fully filled in
	float fCircleIntensity = m_pSettings->nCircleIntensity;
	vecCircles.erase(std::remove_if(vecCircles.begin(), vecCircles.end(),
		[&](cv::Vec3f c)
		{
			if (c[0] < c[2] || c[1] < c[2] || c[0] + c[2] + 1 > m_matRef.cols || c[1] + c[2] + 1 > m_matRef.rows)
				return true;

			cv::Rect rectRoi = cv::Rect(c[0] - c[2], c[1] - c[2], 2 * c[2] + 1, 2 * c[2] + 1);
			cv::Mat roi = matParticle(rectRoi);
			cv::Mat1b mask = cv::Mat::zeros(roi.rows, roi.cols, 0);
			cv::circle(mask, cv::Point(c[2], c[2]), c[2], cv::Scalar(255, 255, 255), -1);
			float intensity = cv::mean(roi, mask)[0];
			return intensity < fCircleIntensity;
		}), vecCircles.end());

	return vecCircles;
}