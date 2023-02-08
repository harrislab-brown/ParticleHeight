#include "TransformSingle.h"
#include <math.h>

using namespace ph;

ph::TransformSingle::TransformSingle(const Particle* p, const Settings* s, bool fastTransform) : m_pParticle(p), m_pSettings(s), m_bFastTransform(fastTransform)
{
	if (fastTransform)
	{
		// precompute the transformed radii
		for (int i = 0; i < 0.707107f * p->getSizeCorrelation() + 1; ++i)
			m_vecTransformedRadii.push_back(Particle::realToPx(m_getTransformedRadiusAnalytic(Particle::pxToReal(i))));
	}
}

bool ph::TransformSingle::operator() (int& pxPosX, int& pxPosY) const
{
	// pxPosX and pxPosY are coordinates with respect to the DIC region
	int midpoint = (m_pParticle->getSizeCorrelation() >> 1);
	float radius = sqrtf((pxPosX - midpoint) * (pxPosX - midpoint) + (pxPosY - midpoint) * (pxPosY - midpoint));
	if (radius == 0)
		return true;  // center point is not transformed
	else if (radius > m_pParticle->getRadiusPx())
		return false;  // point is outside the circle so can't transform it
	else
	{
		// determine the transformed radius
		if (m_bFastTransform)
		{
			float dr = m_vecTransformedRadii[(int)radius + 1] - m_vecTransformedRadii[(int)radius];
			radius = m_vecTransformedRadii[(int)radius] + (radius - (int)radius) * dr;  // interpolate
		}
		else
			radius = Particle::realToPx(m_getTransformedRadiusAnalytic(Particle::pxToReal(radius)));  // exact calculation

		float theta = atan2f(midpoint - pxPosY, pxPosX - midpoint);
		pxPosX = (radius * cosf(theta)) + midpoint;
		pxPosY = midpoint - (radius * sinf(theta));
		return true;
	}
}

float ph::TransformSingle::m_getTransformedRadiusAnalytic(float originalRadius) const
{
	float particleRadius = m_pParticle->getRadiusReal();
	float h = m_pParticle->getPositionReal().z;
	float etaLiquid = m_pSettings->fEtaLiquid;
	float etaParticle = m_pSettings->fEtaParticle;
	float etaGlass = m_pSettings->fEtaGlass;
	float wallThickness = m_pSettings->fChannelWallThickness;

	// intersection between incident ray and sphere
	float theta1 = asinf(originalRadius / particleRadius);
	float theta2 = asinf((etaLiquid * originalRadius) / (etaParticle * particleRadius));
	float theta2p = theta1 - theta2;
	float z = sqrtf(particleRadius * particleRadius - originalRadius * originalRadius) + h;

	// location of second intersection, ray with bottom of circle
	float t1 = tanf(theta2p);
	float t2 = t1 * t1;
	float r1 = (originalRadius - t1 * sqrtf(particleRadius * particleRadius * t2 - h * h * t2 - z * z * t2 -
		originalRadius * originalRadius + particleRadius * particleRadius - 2.0f * h * originalRadius *
		t1 + 2.0f * originalRadius * z * t1 + 2.0f * h * z * t2) + h * t1 - z * t1) / (t2 + 1.0f);
	float z1 = (h - sqrtf(particleRadius * particleRadius * t2 - h * h * t2 - z * z * t2 - originalRadius * 
		originalRadius + particleRadius * particleRadius - 2.0f * h * originalRadius * t1 + 2.0f * 
		originalRadius * z * t1 + 2.0f * h * z * t2) + z * t2 - originalRadius * t1) / (t2 + 1.0f);

	float theta3p = asinf(r1 / particleRadius);
	float theta3 = theta2p + theta3p;

	// third ray, between the sphere and the channel bottom
	float theta4 = asinf(etaParticle / etaLiquid * sinf(theta3));
	float theta4p = theta4 - theta3p;
	float r2 = r1 - (z1 - wallThickness) * tanf(theta4p);

	// final ray in the bottom wall of the channel
	float theta5 = asinf(etaLiquid / etaGlass * sinf(theta4p));
	return r2 - wallThickness * tanf(theta5);
}