#pragma once
#include <fstream>

namespace ph
{
	struct Settings
	{
		Settings() : m_file(nullptr) { m_initializeToDefaults(); };

		void setFile(const char* file) { m_file = file; };
		int load();
		bool save();

		// image processing parameters
		int nBackgroundThreshold;
		int nOpenSize, nOpenIter;
		int nCloseSize, nCloseIter;
		int nCircleMinDist, nCircleThreshold, nCircleParam1, nCircleIntensity;
		int nCircleSize, nCircleSizeRange;
		int nHMaxParam;

		// DIC parameters
		float fPxPerMM;
		float fChannelHeight, fChannelWallThickness;
		float fParticleRadiusPx;
		float fEtaParticle, fEtaLiquid, fEtaGlass;
		int nDICRegionSize;

		// optimizer parameters
		float fXtolAbsSingle;
		float fXtolAbsGroup;
		float fInitStepSingle;
		float fInitStepGroup;
		int nOverlapPenalty;  // coefficient for penalizing overlap during optimization

		// experimental parameters
		float fContactDistance;

	private:
		const char* m_file;

	private:
		void m_initializeToDefaults()
		{
			nBackgroundThreshold = 10;
			nOpenSize = 2;
			nOpenIter = 7;
			nCloseSize = 1;
			nCloseIter = 7;
			nCircleMinDist = 50;
			nCircleThreshold = 4;
			nCircleSize = 50;
			nCircleSizeRange = 8;
			nCircleParam1 = 200;
			nCircleIntensity = 235;
			nHMaxParam = 200;

			fPxPerMM = 62.5f;
			fChannelHeight = 3.0f;
			fChannelWallThickness = 0.951f;
			fParticleRadiusPx = 50.0f;
			fEtaParticle = 1.49569f;
			fEtaLiquid = 1.43935f;
			fEtaGlass = 1.50999f;
			nDICRegionSize = 61;

			fXtolAbsSingle = 0.001;
			fXtolAbsGroup = 0.001;
			fInitStepSingle = 0.1;
			fInitStepGroup = 0.1;
			nOverlapPenalty = 1000;

			fContactDistance = 1.7;
		};
		bool m_setValue(const std::string& key, float value);
		bool m_checkKey(const std::string& key, const std::string& name, bool& success);
		void m_saveSetting(const std::string& name, float value, std::ofstream& file);
	};
}

