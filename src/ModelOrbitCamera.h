#pragma once

#include "Camera.h"
#include <glm/glm.hpp>

class ModelOrbitCamera : public Camera
{
public:
	ModelOrbitCamera();
	ModelOrbitCamera(ModelOrbitCamera&&) = default;
	virtual ~ModelOrbitCamera() {};

	static constexpr const char* identifier = "model_orbit";

	virtual void reset() override;
	virtual void setup() override;
	void setupSecondary();

	virtual void key(float change_x, float change_y, bool alternative, float factor) override;

	virtual void leftMouseStart() override;
	virtual void leftMouse(float change_x, float change_y, float factor) override;
	virtual void leftMouseEnd() override;

	virtual void rightMouseStart() override;
	virtual void rightMouse(float change_x, float change_y, float factor) override;
	virtual void rightMouseEnd() override;

	virtual void scroll(float change, float factor) override;

	void updateFocus(const glm::vec3& center, float boundsRadius, bool updateCenter);
	void resetView();
	void resetSecondaryView();
	void mirrorSecondaryToOpposite();

	const glm::vec3& focus() const { return m_focus; }
	void setFocus(const glm::vec3& center);

	const glm::vec3& orbitOffset() const { return m_orbitOffset; }
	void setOrbitOffset(const glm::vec3& offset);

	const glm::vec3& secondaryOrbitOffset() const { return m_secondaryOrbitOffset; }
	void setSecondaryOrbitOffset(const glm::vec3& offset);

	const glm::vec3& upVector() const { return m_upVector; }
	void setUpVector(const glm::vec3& up);

	float minDistance() const { return m_minDistance; }
	void setMinDistance(float distance);

	float maxDistance() const { return m_maxDistance; }
	void setMaxDistance(float distance);

	float distance() const;
	float azimuthDegrees() const;
	float elevationDegrees() const;
	void setOrbitSpherical(float azimuthDegrees, float elevationDegrees, float orbitDistance);

	float secondaryDistance() const;
	float secondaryAzimuthDegrees() const;
	float secondaryElevationDegrees() const;
	void setSecondaryOrbitSpherical(float azimuthDegrees, float elevationDegrees, float orbitDistance);
	void orbitSecondary(float change_x, float change_y, float factor);

	glm::vec3 eyePosition() const;
	void setEyePosition(const glm::vec3& eye);

	glm::vec3 secondaryEyePosition() const;
	void setSecondaryEyePosition(const glm::vec3& eye);

protected:
	void orbit(float change_x, float change_y, float factor);
	void applyView(const glm::vec3& eye);
	void clampOrbitOffset();
	void clampSecondaryOrbitOffset();
	glm::vec3 clampOffsetLength(const glm::vec3& offset) const;

	float azimuthDegreesFor(const glm::vec3& offset) const;
	float elevationDegreesFor(const glm::vec3& offset) const;
	void setSphericalOnOffset(glm::vec3& offset, float azimuthDegrees, float elevationDegrees, float orbitDistance);

	glm::vec3 m_focus;
	glm::vec3 m_orbitOffset;
	glm::vec3 m_secondaryOrbitOffset;
	glm::vec3 m_upVector;

	float m_minDistance;
	float m_maxDistance;
	bool m_initialized;
	bool m_secondaryInitialized;
};
