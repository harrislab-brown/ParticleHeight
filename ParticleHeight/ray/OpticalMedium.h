#pragma once
#include "util/vf3.h"
#include "Ray.h"

namespace ph
{
	class OpticalMedium
	{
	public:
		OpticalMedium(float refractionIndex) : refractionIndex(refractionIndex) {};
		OpticalMedium() : refractionIndex(1) {};
		~OpticalMedium() {};
	public:
		virtual bool isTerminal() const { return false; };
		virtual bool isParticle() const { return false; };
		virtual vf3 getNormal(const vf3& location) const = 0;
		virtual bool getIntersection(float& distance, const Ray& ray) const = 0;
		virtual bool containsPoint(const vf3& point) const = 0;
	public:
		float refractionIndex;
	};
}

