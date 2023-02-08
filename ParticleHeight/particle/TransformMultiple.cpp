#include "TransformMultiple.h"
#include "TransformSingle.h"

using namespace ph;

bool ph::TransformMultiple::operator() (int& pxPosX, int& pxPosY) const
{
	// pxPosX and pxPosY are coordinates with respect to the DIC region
	float particlePosX, particlePosY;
	m_pParticle->getPositionPx(particlePosX, particlePosY);

	// translate to overall channel coordinates
	float channelPosX = Particle::pxToReal(pxPosX + particlePosX - (m_pParticle->getSizeCorrelation() >> 1));
	float channelPosY = Particle::pxToReal(pxPosY + particlePosY - (m_pParticle->getSizeCorrelation() >> 1));

	// if the pixel in question only contains one particle, use the analytical transformation
	if (m_pScene->posOverlapsSeveralParticles(channelPosX, channelPosY))
	{
		// create the ray
		Ray r(vf3(channelPosX, channelPosY, 5), vf3(0, 0, -1));

		// propagate the ray through the scene
		vf3 t = m_pScene->getRayTermination(r);

		// translate back to DIC region coordinates
		pxPosX = (Particle::realToPx(t.x) - particlePosX + (m_pParticle->getSizeCorrelation() >> 1));
		pxPosY = (Particle::realToPx(t.y) - particlePosY + (m_pParticle->getSizeCorrelation() >> 1));
	}
	else
	{
		// use analytical transformation
		TransformSingle t(m_pParticle, m_pSettings);
		t(pxPosX, pxPosY);
	}

	return true;
}