#pragma once
#include "util/vf3.h"
#include "util/Settings.h"
#include <vector>

namespace ph
{
	class Particle
	{
	public:
		Particle(float radius, float pxPosX, float pxPosY, float unused);
		Particle() : m_pxCircleRadius(m_pSettings->fParticleRadiusPx), m_pxCirclePosX(0), m_pxCirclePosY(0), 
			m_vPosition(vf3(0, 0, m_pSettings->fChannelWallThickness + 0.5 * m_pSettings->fChannelHeight)), 
			m_bHeightKnown(false), m_fConfidence(0), m_nSizeCorrelation(m_pSettings->nDICRegionSize) {};
		~Particle() {};
	public:
		bool isOverlapping(const Particle&) const;
		bool isHeightKnown() const { return m_bHeightKnown; };
		void setHeightKnown(bool h) { m_bHeightKnown = h; };
		void setPosition(vf3 newPosition) { m_vPosition = newPosition; m_pxCirclePosX = realToPx(newPosition.x); m_pxCirclePosY = realToPx(newPosition.y); };
		void setPosition(float pxPosX, float pxPosY) { m_pxCirclePosX = pxPosX; m_pxCirclePosY = pxPosY; m_vPosition.x = pxToReal(pxPosX); m_vPosition.y = pxToReal(pxPosY); };
		vf3 getPositionReal() const { return m_vPosition; };
		void getPositionPx(float& pxPosX, float& pxPosY) const { pxPosX = m_pxCirclePosX; pxPosY = m_pxCirclePosY; };
		float getRadiusPx() const { return m_pxCircleRadius; };
		float getRadiusReal() const { return pxToReal(m_pxCircleRadius); };
		void setConfidence(float c) { m_fConfidence = c; };
		float getConfidence() const { return m_fConfidence; };
		void setSizeCorrelation(int s) { m_nSizeCorrelation = s; };
		int getSizeCorrelation() const { return m_nSizeCorrelation; };
		bool hasNeighbors() const { return !m_vecNeighbors.empty(); };
		void addNeighbor(Particle* p) { m_vecNeighbors.push_back(p); };
		std::vector<Particle*> getNeighbors() const { return m_vecNeighbors; };
		void getOverlapGroup(std::vector<Particle*>&);
		float getCenterDist(const Particle&) const;
	private:
		float m_pxCircleRadius;
		float m_pxCirclePosX, m_pxCirclePosY;
		vf3 m_vPosition;
		bool m_bHeightKnown;
		float m_fConfidence;
		int m_nSizeCorrelation;
		std::vector<Particle*> m_vecNeighbors;  // particles immediately connected to this one
	private:
		static const Settings* m_pSettings;
	public:
		static void setSettings(const Settings* s) { m_pSettings = s; };
		static float pxToReal(float px) { return px / m_pSettings->fPxPerMM; };
		static float realToPx(float real) { return real * m_pSettings->fPxPerMM; };
	};
}
