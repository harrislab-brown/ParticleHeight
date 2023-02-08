#include "Particle.h"

using namespace ph;

const Settings* Particle::m_pSettings = nullptr;

ph::Particle::Particle(float radius, float pxPosX, float pxPosY, float unused) : m_pxCircleRadius(radius), m_bHeightKnown(false), m_fConfidence(0)
{
	setPosition(pxPosX, pxPosY);
}

bool ph::Particle::isOverlapping(const Particle& other) const
{
	float pxPosXOther, pxPosYOther;
	other.getPositionPx(pxPosXOther, pxPosYOther);
	float distance = (m_pxCirclePosX - pxPosXOther) * (m_pxCirclePosX - pxPosXOther) + (m_pxCirclePosY - pxPosYOther) * (m_pxCirclePosY - pxPosYOther);
	float overlapDistance = (m_pxCircleRadius + other.getRadiusPx()) * (m_pxCircleRadius + other.getRadiusPx());

	return distance < overlapDistance;
}

void ph::Particle::getOverlapGroup(std::vector<Particle*>& vecpGroup)
{
	// add this to the group
	vecpGroup.push_back(this);

	// depth first traversal to find all the connected particles
	for (auto n : m_vecNeighbors)
		if (std::find(vecpGroup.begin(), vecpGroup.end(), n) == vecpGroup.end())  // only visit the particles that aren't already in the group
			n->getOverlapGroup(vecpGroup);
}

float ph::Particle::getCenterDist(const Particle& other) const
{
	return (this->getPositionReal() - other.getPositionReal()).mag();
}
