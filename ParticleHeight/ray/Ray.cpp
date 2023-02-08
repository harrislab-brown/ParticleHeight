#include "Ray.h"
#include <math.h>

using namespace ph;

const float ph::Ray::m_bias = 1e-4f;

bool ph::Ray::refractIntoNewMedium(vf3 normal, float newRefractionIndex)
{
	vf3 newDirection;
	if (m_getRefractionDirection(newDirection, normal, newRefractionIndex))
	{
		// update ray direction
		m_direction = newDirection.normalize();

		// update refraction index
		setRefractionIndex(newRefractionIndex);

		// propagate slightly to avoid intersecting same object
		propagate(m_bias);

		return true;
	}
	else
	{
		return false;
	}
}

bool ph::Ray::m_getRefractionDirection(vf3& refractedDirection, vf3 normal, float newRefractionIndex) const
{
	// Snell's law in 3D
   	float r = m_currentRefractionIndex / newRefractionIndex;  // ratio of refraction indices
	normal.normalizeInPlace();
	vf3 incident = m_direction.normalize();

	float c = -normal.dot(incident);
	if (c < 0)
	{
		// normal direction is wrong so switch it
		normal *= -1;
		c *= -1;
	}

	float disc = 1 - r * r * (1 - c * c);
	if (disc < 0)
	{
		// total internal reflection so no refraction
		return false;
	}
	else
	{
		// refraction occurs
		refractedDirection = incident * r + normal * (r * c - sqrtf(disc));
		return true;
	}
}

