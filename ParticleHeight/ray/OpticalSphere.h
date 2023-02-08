#pragma once
#include "OpticalMedium.h"

namespace ph
{
	class OpticalSphere : public OpticalMedium
	{
		// a sphere
	public:
		OpticalSphere(vf3 c, float r, float eta) : m_vCenter(c), m_radius(r), OpticalMedium(eta) {};
		~OpticalSphere() {};
	public:
		virtual bool isParticle() const { return true; };
		virtual vf3 getNormal(const vf3& location) const;
		virtual bool getIntersection(float& distance, const Ray& ray) const;
		virtual bool containsPoint(const vf3& point) const;
	public:
		void setPosition(const vf3& pos) { m_vCenter = pos; };
	public:
		bool overlapsPoint(float posX, float posY) const { return (vf3(posX, posY, m_vCenter.z) - m_vCenter).mag() < m_radius; };
	private:
		vf3 m_vCenter;
		float m_radius;
	};
}
