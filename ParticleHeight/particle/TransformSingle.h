#pragma once
#include "Particle.h"
#include "util/Settings.h"

namespace ph
{
	class TransformSingle
	{
	public:
		TransformSingle(const Particle* p, const Settings* s, bool fastTransform = false);
		TransformSingle() : m_pParticle(nullptr), m_pSettings(nullptr), m_bFastTransform(false) {};
		~TransformSingle() {};
	public:
		bool operator() (int& pxPosX, int& pxPosY) const;
	private:
		float m_getTransformedRadiusAnalytic(float originalRadius) const;
	private:
		const Particle* m_pParticle;
		const Settings* m_pSettings;
		bool m_bFastTransform;
		std::vector<float> m_vecTransformedRadii;
	};
}
