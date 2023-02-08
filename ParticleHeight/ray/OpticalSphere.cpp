#include "OpticalSphere.h"
#include <math.h>

using namespace ph;

vf3 ph::OpticalSphere::getNormal(const vf3& location) const 
{
	return (location - m_vCenter).normalize();
}

bool ph::OpticalSphere::getIntersection(float& distance, const Ray& ray) const 
{
	float a = ray.getDirection().square();
	float b = 2 * ray.getDirection().dot(ray.getOrigin() - m_vCenter);
	float c = (ray.getOrigin() - m_vCenter).square() - m_radius * m_radius;
	float disc = b * b - 4 * a * c;

	if (disc < 0)
	{
		// no collisions
		return false;
	}
	else
	{
		// possible distances to a collision
		float r = sqrtf(disc);
		float d1 = (-b + r) / (2 * a);
		float d2 = (-b - r) / (2 * a);

		// want the smallest positive value for distance to collision
		if (d1 < 0)
		{
			// intersections in opposite direction to the ray
			return false;
		}
		else
		{
			// at least one valid intersection
			if (d2 > 0)
			{
				// both intersections valid, d2 is the smaller number
				distance = d2;
				return true;
			}
			else
			{
				// only d1 is valid so return it
				distance = d1;
				return true;
			}
		}
	}
}

bool ph::OpticalSphere::containsPoint(const vf3& point) const 
{
	return (point - m_vCenter).mag() < m_radius;
}