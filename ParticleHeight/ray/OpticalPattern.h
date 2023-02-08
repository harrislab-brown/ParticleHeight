#pragma once
#include "OpticalMedium.h"
#include "util/vf3.h"

namespace ph
{
	class OpticalPattern : public OpticalMedium
	{
		// a plane at which rays should terminate
	public:
		OpticalPattern(vf3 p, vf3 n) : m_vPoint(p), m_vNormal(n) {};
		OpticalPattern() {};
		~OpticalPattern() {};
	public:
		virtual bool isTerminal() const { return true; };
		virtual vf3 getNormal(const vf3& location) const;
		virtual bool getIntersection(float& distance, const Ray& ray) const;
		virtual bool containsPoint(const vf3& point) const;
	private:
		vf3 m_vPoint, m_vNormal;
	};
}
