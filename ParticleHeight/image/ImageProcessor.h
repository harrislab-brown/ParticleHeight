#pragma once
#include <opencv2/core/mat.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/video/tracking.hpp>
#include <opencv2/video/background_segm.hpp>
#include "util/Settings.h"

namespace ph
{
	class ImageProcessor
	{
	public:
		ImageProcessor(cv::Mat matRef, const Settings* s) : m_matRef(matRef), m_typeRef(matRef.depth()), m_pSettings(s) {};
		ImageProcessor() : m_typeRef(0), m_pSettings(nullptr) {};
		~ImageProcessor() {};
	public:
		void setRef(cv::Mat matRef) { m_matRef = matRef; };
		void setSettings(const Settings* s) { m_pSettings = s; };
	public:
		double alignToRef(cv::Mat matParticle) const;
		void subtractBackground(cv::Mat matParticle) const;
		void morphOpen(cv::Mat matParticle) const;
		void morphClose(cv::Mat matParticle) const;
		cv::Mat morphReconstruct(cv::Mat matMarker, cv::Mat matMask) const;
		std::vector<cv::Vec3f> findCirclesHough(cv::Mat matParticle) const;
		std::vector<cv::Vec3f> findCirclesEDT(cv::Mat matParticle) const;  // more robust circle finding

		// template member functions for use with functors
		template <class F>
		cv::Mat transformRef(cv::Rect rectRegion, F transform) const
		{
			// return a new Mat which is the region of the ref image specified by rectRegion transformed by the function transform
			// transform should take an x, y coordinate of the transformed image and return by reference where in the ref image to get that pixel value
			// transform returns true if the transformation is possible and false if not (e.g. if a ray is cast beyond the bounds of the ref image)

			// create the output matrix and initialize to all zeros
			cv::Mat matOutput(rectRegion.size(), CV_8UC1, cv::Scalar(0));

			// set each pixel of the output matrix using opencv::mat's forEach (parallelized)
			matOutput.forEach<Pixel>
				(
					[&](Pixel& pixel, const int* position) -> void
					{
						int pxPosY = position[0];  // these are flipped because the Mat object is indexed row, col
						int pxPosX = position[1];
						// try transforming that position and make sure the resulting location is within bounds of ref image
						if (transform(pxPosX, pxPosY) && pxPosX + rectRegion.x >= 0 && pxPosY + rectRegion.y >= 0 && pxPosX + rectRegion.x < m_matRef.cols && pxPosY + rectRegion.y < m_matRef.rows)
							pixel = m_matRef.at<uint8_t>(pxPosY + rectRegion.y, pxPosX + rectRegion.x);  // at is indexed row, column (y, x)
					}
			);

			return matOutput;
		}

		template <class F>
		float correlateTransform(F transform, cv::Rect rectRegion, const cv::Mat matParticle) const
		{
			cv::Mat matTransIm = transformRef(rectRegion, transform);

			cv::Mat matCorrelation;
			cv::matchTemplate(matParticle(rectRegion), matTransIm, matCorrelation, cv::TM_CCOEFF_NORMED);
			return matCorrelation.at<float>(0, 0);
		}

	private:
		cv::Mat m_matRef;
		int m_typeRef;  // array type of the reference image
		typedef uint8_t Pixel;  // image type here should be 1 channel CV_8U image
		const Settings* m_pSettings;
	};
}
