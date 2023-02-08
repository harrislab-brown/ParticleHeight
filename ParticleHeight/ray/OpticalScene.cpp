#include "OpticalScene.h"
#include "OpticalSphere.h"

using namespace ph;

vf3 ph::OpticalScene::getRayTermination(Ray& ray) const
{
	m_updateRayRefractionIndex(ray);
	return m_traceRay(ray);
}

vf3 ph::OpticalScene::m_traceRay(Ray& ray, unsigned int depth) const
{
	// recursively trace the ray until it either hits a termination or runs out of depth
	if (depth == 0)
		return ray.getOrigin();

	// find the first object the ray hits
	std::shared_ptr<OpticalMedium> pIntersectedObject = nullptr;
	float fIntersectDistance, fMinDistance = INFINITY;
	for (auto medium : m_vecpOpticalMedia)
		if (medium->getIntersection(fIntersectDistance, ray) && fIntersectDistance < fMinDistance)
		{
			pIntersectedObject = medium;
			fMinDistance = fIntersectDistance;
		}

	if (pIntersectedObject == nullptr)
	{
		// no object hit
		return ray.getOrigin();
	}
	else
	{
		// store the new object refraction index now since it requires previous ray location to determine if we are entering or exiting an object
		float newRefractionIndex = pIntersectedObject->containsPoint(ray.getOrigin()) ? m_refractionIndex : pIntersectedObject->refractionIndex;  

		// an object is hit so update ray location
		ray.propagate(fMinDistance);
		if (pIntersectedObject->isTerminal())
			return ray.getOrigin();
		else
		{
			// try refracting the ray
			vf3 normal = pIntersectedObject->getNormal(ray.getOrigin());
			if (ray.refractIntoNewMedium(normal, newRefractionIndex))
			{
				// recursive call
				return m_traceRay(ray, --depth);
			}
			else
			{
				// total internal reflection
				return ray.getOrigin();
			}
		}
	}
}

void ph::OpticalScene::m_updateRayRefractionIndex(Ray& ray) const
{
	//set the ray's current refraction index depending on its location in the scene
	ray.setRefractionIndex(this->m_refractionIndex);

	for (auto pMedium : m_vecpOpticalMedia)
	{
		if (pMedium->containsPoint(ray.getOrigin()))
		{
			ray.setRefractionIndex(pMedium->refractionIndex);
			break;
		}
	}
}

bool ph::OpticalScene::posOverlapsSeveralParticles(float posX, float posY) const
{
	int nOverlaps = 0;
	for (auto pMedium : m_vecpOpticalMedia)
		if (pMedium->isParticle())
		{
			OpticalSphere* s = (OpticalSphere*)pMedium.get();
			if (s->overlapsPoint(posX, posY) && ++nOverlaps > 1) return true;
		}

	return false;
}