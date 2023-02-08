#pragma once
#include <vector>
#include <memory>
#include "OpticalMedium.h"
#include "Ray.h"
#include "util/vf3.h"

namespace ph
{
	class OpticalScene
	{
	public:
		OpticalScene(float refractionIndex) : m_refractionIndex(refractionIndex) {};
		OpticalScene() : m_refractionIndex(1) {};
		~OpticalScene() {};
	public:
		void addMedium(std::shared_ptr<OpticalMedium> pOpticalObject) { m_vecpOpticalMedia.push_back(pOpticalObject); };
		std::vector<std::shared_ptr<OpticalMedium>>& getMedia() { return m_vecpOpticalMedia; };
		vf3 getRayTermination(Ray& ray) const;
	public:
		bool posOverlapsSeveralParticles(float posX, float posY) const;
	private:
		vf3 m_traceRay(Ray& ray, unsigned int depth = 10) const;
		void m_updateRayRefractionIndex(Ray& ray) const;
	private:
		std::vector<std::shared_ptr<OpticalMedium>> m_vecpOpticalMedia;
		float m_refractionIndex;
	};
}

