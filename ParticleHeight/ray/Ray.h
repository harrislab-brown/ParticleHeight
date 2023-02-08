#pragma once
#include "util/vf3.h"

namespace ph
{
	class Ray
	{
	public:
		Ray(vf3 origin, vf3 direction) : m_origin(origin), m_direction(direction), m_currentRefractionIndex(1) {};
		Ray() : m_origin(vf3()), m_direction(vf3()), m_currentRefractionIndex(1) {};
		~Ray() {};
	public:
		bool refractIntoNewMedium(vf3 normal, float newRefractionIndex);
		void propagate(float distance) { m_origin += m_direction * distance; };
		void setRefractionIndex(float refractionIndex) { m_currentRefractionIndex = refractionIndex; };
		vf3 getOrigin() const { return m_origin; };
		vf3 getDirection() const { return m_direction; };
	private:
		bool m_getRefractionDirection(vf3& refractedDirection, vf3 normal, float newRefractionIndex) const;
	private:
		vf3 m_origin, m_direction;
		float m_currentRefractionIndex;
		static const float m_bias;
	};
}

