#pragma once
#include <math.h>

namespace ph
{
	class vf3
	{
	public:
		vf3(float x, float y, float z) : x(x), y(y), z(z) {};
		vf3() : x(0), y(0), z(0) {};
		vf3(const vf3& v) : x(v.x), y(v.y), z(v.z) {};  // copy constructor
		~vf3() {};  // destructor
	public:
		float x, y, z;
	public:
		float mag() const { return sqrtf(x * x + y * y + z * z); };
		vf3 normalize() const { float r = 1 / mag(); return vf3(r * x, r * y, r * z); };
		vf3& normalizeInPlace() { float r = 1 / mag(); this->x *= r; this->y *= r; this->z *= r; return *this; };
		float dot(const vf3& rhs) const { return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z; };
		vf3 cross(const vf3& rhs) const { return vf3(this->y * rhs.z - this->z * rhs.y, this->z * rhs.x - this->x * rhs.z, this->x * rhs.y - this->y * rhs.x); };
		float square() const { return this->dot(*this); };
	public:
		vf3 operator + (const vf3& rhs) const { return vf3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z); };
		vf3 operator - (const vf3& rhs) const { return vf3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z); };
		vf3 operator * (const float& rhs) const { return vf3(this->x * rhs, this->y * rhs, this->z * rhs); };
		vf3 operator / (const float& rhs) const { return vf3(this->x / rhs, this->y / rhs, this->z / rhs); };
		vf3& operator += (const vf3& rhs) { this->x += rhs.x; this->y += rhs.y; this->z += rhs.z; return *this; };
		vf3& operator -= (const vf3& rhs) { this->x -= rhs.x; this->y -= rhs.y; this->z -= rhs.z; return *this; };
		vf3& operator *= (const float& rhs) { this->x *= rhs; this->y *= rhs; this->z *= rhs; return *this; };
		vf3& operator /= (const float& rhs) { this->x /= rhs; this->y /= rhs; this->z /= rhs; return *this; };
	};
}
