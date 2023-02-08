#pragma once
#include "Particle.h"
#include "ray/OpticalScene.h"
#include "ray/Ray.h"
#include "util/vf3.h"

namespace ph
{
	class TransformMultiple
	{
	public:
		TransformMultiple(const Particle* p, const Settings* s, OpticalScene* sc) : m_pParticle(p), m_pSettings(s), m_pScene(sc) {};
		TransformMultiple() : m_pParticle(nullptr), m_pSettings(nullptr), m_pScene(nullptr) {};
		~TransformMultiple() {};
	public:
		bool operator() (int& pxPosX, int& pxPosY) const;
	private:
		const Particle* m_pParticle;
		const Settings* m_pSettings;
		OpticalScene* m_pScene;
	};
}
