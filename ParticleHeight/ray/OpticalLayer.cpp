#include "OpticalLayer.h"
#include <cmath>

using namespace ph;

vf3 ph::OpticalLayer::getNormal(const vf3& location) const
{
	return m_vNormal;
}

bool ph::OpticalLayer::getIntersection(float& distance, const Ray& ray) const
{
	float a = ray.getDirection().dot(m_vNormal);

	if (a == 0)
	{
		// ray is parallel with planes
		return false;
	}
	else
	{
		// could intersect with either plane
		float d1 = m_vNormal.dot(m_vPoint1 - ray.getOrigin()) / a;
		float d2 = m_vNormal.dot(m_vPoint2 - ray.getOrigin()) / a;

		// pick the correct distance
		if (d1 < 0)
			if (d2 < 0)
				return false;
			else
			{
				distance = d2; 
				return true;
			}
		else
			if (d2 < 0)
			{
				distance = d1;
				return true;
			}
			else
			{
				distance = d1 < d2 ? d1 : d2;
				return true;
			}
	}
}

bool ph::OpticalLayer::containsPoint(const vf3& point) const
{
	return std::signbit(m_vNormal.dot(point - m_vPoint1)) != std::signbit(m_vNormal.dot(point - m_vPoint2));
}