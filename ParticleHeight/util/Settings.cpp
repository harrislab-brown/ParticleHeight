#include "Settings.h"
#include <fstream>

using namespace ph;

int ph::Settings::load()
{
	int nSettingsLoaded = 0;
	std::ifstream settingsFile(m_file);

	if (!settingsFile.is_open())
		return -1;

	std::string key;
	float value;
	while (settingsFile >> key >> value)
		if (m_setValue(key, value))
			nSettingsLoaded++;

	settingsFile.close();
	return nSettingsLoaded;
}

bool ph::Settings::save()
{
	std::ofstream settingsFile(m_file);

	if (!settingsFile.is_open())
		return false;

	m_saveSetting("BackgroundThreshold", nBackgroundThreshold, settingsFile);
	m_saveSetting("OpenSize", nOpenSize, settingsFile);
	m_saveSetting("OpenIter", nOpenIter, settingsFile);
	m_saveSetting("CloseSize", nCloseSize, settingsFile);
	m_saveSetting("CloseIter", nCloseIter, settingsFile);
	m_saveSetting("CircleMinDist", nCircleMinDist, settingsFile);
	m_saveSetting("CircleThreshold", nCircleThreshold, settingsFile);
	m_saveSetting("CircleParam1", nCircleParam1, settingsFile);
	m_saveSetting("CircleIntensity", nCircleIntensity, settingsFile);
	m_saveSetting("CircleSize", nCircleSize, settingsFile);
	m_saveSetting("CircleSizeRange", nCircleSizeRange, settingsFile);
	m_saveSetting("HMaxParam", nHMaxParam, settingsFile);

	m_saveSetting("PxPerMM", fPxPerMM, settingsFile);
	m_saveSetting("ChannelHeight", fChannelHeight, settingsFile);
	m_saveSetting("ChannelWallThickness", fChannelWallThickness, settingsFile);
	m_saveSetting("ParticleRadiusPx", fParticleRadiusPx, settingsFile);
	m_saveSetting("EtaParticle", fEtaParticle, settingsFile);
	m_saveSetting("EtaLiquid", fEtaLiquid, settingsFile);
	m_saveSetting("EtaGlass", fEtaGlass, settingsFile);
	m_saveSetting("DICRegionSize", nDICRegionSize, settingsFile);

	m_saveSetting("XtolAbsSingle", fXtolAbsSingle, settingsFile);
	m_saveSetting("XtolAbsGroup", fXtolAbsGroup, settingsFile);
	m_saveSetting("InitStepSingle", fInitStepSingle, settingsFile);
	m_saveSetting("InitStepGroup", fInitStepGroup, settingsFile);
	m_saveSetting("OverlapPenalty", nOverlapPenalty, settingsFile);

	settingsFile.close();
	return true;
}

bool ph::Settings::m_setValue(const std::string& key, float value)
{
	bool success = false;

	if (m_checkKey(key, "BackgroundThreshold", success)) nBackgroundThreshold = value;
	if (m_checkKey(key, "OpenSize", success)) nOpenSize = value;
	if (m_checkKey(key, "OpenIter", success)) nOpenIter = value;
	if (m_checkKey(key, "CloseSize", success)) nCloseSize = value;
	if (m_checkKey(key, "CloseIter", success)) nCloseIter = value;
	if (m_checkKey(key, "CircleMinDist", success)) nCircleMinDist = value;
	if (m_checkKey(key, "CircleThreshold", success)) nCircleThreshold = value;
	if (m_checkKey(key, "CircleParam1", success)) nCircleParam1 = value;
	if (m_checkKey(key, "CircleIntensity", success)) nCircleIntensity = value;
	if (m_checkKey(key, "CircleSize", success)) nCircleSize = value;
	if (m_checkKey(key, "CircleSizeRange", success)) nCircleSizeRange = value;
	if (m_checkKey(key, "HMaxParam", success)) nHMaxParam = value;

	if (m_checkKey(key, "PxPerMM", success)) fPxPerMM = value;
	if (m_checkKey(key, "ChannelHeight", success)) fChannelHeight = value;
	if (m_checkKey(key, "ChannelWallThickness", success)) fChannelWallThickness = value;
	if (m_checkKey(key, "ParticleRadiusPx", success)) fParticleRadiusPx = value;
	if (m_checkKey(key, "EtaParticle", success)) fEtaParticle = value;
	if (m_checkKey(key, "EtaLiquid", success)) fEtaLiquid = value;
	if (m_checkKey(key, "EtaGlass", success)) fEtaGlass = value;
	if (m_checkKey(key, "DICRegionSize", success)) nDICRegionSize = value;

	if (m_checkKey(key, "XtolAbsSingle", success)) fXtolAbsSingle = value;
	if (m_checkKey(key, "XtolAbsGroup", success)) fXtolAbsGroup = value;
	if (m_checkKey(key, "InitStepSingle", success)) fInitStepSingle = value;
	if (m_checkKey(key, "InitStepGroup", success)) fInitStepGroup = value;
	if (m_checkKey(key, "OverlapPenalty", success)) nOverlapPenalty = value;

	return success;
}

bool ph::Settings::m_checkKey(const std::string& key, const std::string& name, bool& success)
{
	success |= (key == name);
	return (key == name);
}

void ph::Settings::m_saveSetting(const std::string& name, float value, std::ofstream& file)
{
	file << name << " " << value << std::endl;
}
