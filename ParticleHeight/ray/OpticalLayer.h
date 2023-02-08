#pragma once
#include "OpticalMedium.h"
#include "util/vf3.h"

namespace ph
{
	class OpticalLayer : public OpticalMedium
	{
		// defined as the region between 2 parallel planes
	public:
		OpticalLayer(vf3 p1, vf3 p2, vf3 n, float eta) : m_vPoint1(p1), m_vPoint2(p2), m_vNormal(n), OpticalMedium(eta) {};
		OpticalLayer() {};
		~OpticalLayer() {};
	public:
		virtual vf3 getNormal(const vf3& location) const;
		virtual bool getIntersection(float& distance, const Ray& ray) const;
		virtual bool containsPoint(const vf3& point) const;
	private:
		vf3 m_vPoint1, m_vPoint2, m_vNormal;
	};
}

