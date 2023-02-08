#include "OpticalPattern.h"

using namespace ph;

vf3 ph::OpticalPattern::getNormal(const vf3& location) const
{
	return m_vNormal;
}

bool ph::OpticalPattern::getIntersection(float& distance, const Ray& ray) const
{
	float a = ray.getDirection().dot(m_vNormal);

	if (a == 0)
	{
		// ray is parallel to plane
		return false;
	}
	else
	{
		float d = m_vNormal.dot(m_vPoint - ray.getOrigin()) / a;
		
		if (d < 0)
		{
			return false;
		}
		else
		{
			distance = d;
			return true;
		}
	}
}

bool ph::OpticalPattern::containsPoint(const vf3& point) const
{
	// a plane so can't contain a point
	return false;
}