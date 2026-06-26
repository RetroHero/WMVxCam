#include "stdafx.h"
#include "ModelOrbitCamera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

ModelOrbitCamera::ModelOrbitCamera()
{
	reset();
}

void ModelOrbitCamera::reset()
{
	m_focus = glm::vec3{ 0.f, 0.f, 0.f };
	m_orbitOffset = glm::vec3{ 0.f, 0.f, 2.f };
	m_secondaryOrbitOffset = glm::vec3{ 0.f, 0.f, -2.f };
	m_upVector = glm::vec3{ 0.f, 1.f, 0.f };
	m_minDistance = 0.5f;
	m_maxDistance = 100.f;
	m_initialized = false;
	m_secondaryInitialized = false;
}

void ModelOrbitCamera::resetView()
{
	m_initialized = false;
	m_secondaryInitialized = false;
}

void ModelOrbitCamera::resetSecondaryView()
{
	m_secondaryInitialized = false;
}

void ModelOrbitCamera::mirrorSecondaryToOpposite()
{
	m_secondaryOrbitOffset = -m_orbitOffset;
	clampSecondaryOrbitOffset();
	m_secondaryInitialized = true;
}

void ModelOrbitCamera::setFocus(const glm::vec3& center)
{
	m_focus = center;
}

void ModelOrbitCamera::setOrbitOffset(const glm::vec3& offset)
{
	m_orbitOffset = offset;
	clampOrbitOffset();
}

void ModelOrbitCamera::setSecondaryOrbitOffset(const glm::vec3& offset)
{
	m_secondaryOrbitOffset = offset;
	clampSecondaryOrbitOffset();
}

void ModelOrbitCamera::setUpVector(const glm::vec3& up)
{
	if (glm::length(up) < 1e-4f) {
		return;
	}
	m_upVector = glm::normalize(up);
}

void ModelOrbitCamera::setMinDistance(float distance)
{
	m_minDistance = std::max(distance, 0.01f);
	clampOrbitOffset();
	clampSecondaryOrbitOffset();
}

void ModelOrbitCamera::setMaxDistance(float distance)
{
	m_maxDistance = std::max(distance, m_minDistance);
	clampOrbitOffset();
	clampSecondaryOrbitOffset();
}

glm::vec3 ModelOrbitCamera::clampOffsetLength(const glm::vec3& offset) const
{
	const float len = glm::length(offset);
	if (len < 1e-4f) {
		return glm::vec3{ 0.f, 0.f, m_minDistance };
	}

	const float clamped = std::clamp(len, m_minDistance, m_maxDistance);
	return glm::normalize(offset) * clamped;
}

float ModelOrbitCamera::azimuthDegreesFor(const glm::vec3& offset) const
{
	const float dist = glm::length(offset);
	if (dist < 1e-4f) {
		return 0.f;
	}
	return glm::degrees(std::atan2(offset.x, offset.z));
}

float ModelOrbitCamera::elevationDegreesFor(const glm::vec3& offset) const
{
	const float dist = glm::length(offset);
	if (dist < 1e-4f) {
		return 0.f;
	}
	return glm::degrees(std::asin(std::clamp(offset.y / dist, -1.f, 1.f)));
}

void ModelOrbitCamera::setSphericalOnOffset(glm::vec3& offset, float azimuthDegrees, float elevationDegrees, float orbitDistance)
{
	const float azimuth = glm::radians(azimuthDegrees);
	const float elevation = glm::radians(elevationDegrees);
	const float horizontal = orbitDistance * std::cos(elevation);

	offset = glm::vec3{
		horizontal * std::sin(azimuth),
		orbitDistance * std::sin(elevation),
		horizontal * std::cos(azimuth)
	};
}

float ModelOrbitCamera::distance() const
{
	return glm::length(m_orbitOffset);
}

float ModelOrbitCamera::azimuthDegrees() const
{
	return azimuthDegreesFor(m_orbitOffset);
}

float ModelOrbitCamera::elevationDegrees() const
{
	return elevationDegreesFor(m_orbitOffset);
}

float ModelOrbitCamera::secondaryDistance() const
{
	return glm::length(m_secondaryOrbitOffset);
}

float ModelOrbitCamera::secondaryAzimuthDegrees() const
{
	return azimuthDegreesFor(m_secondaryOrbitOffset);
}

float ModelOrbitCamera::secondaryElevationDegrees() const
{
	return elevationDegreesFor(m_secondaryOrbitOffset);
}

void ModelOrbitCamera::setOrbitSpherical(float azimuthDegrees, float elevationDegrees, float orbitDistance)
{
	setSphericalOnOffset(m_orbitOffset, azimuthDegrees, elevationDegrees, orbitDistance);
	clampOrbitOffset();
}

void ModelOrbitCamera::setSecondaryOrbitSpherical(float azimuthDegrees, float elevationDegrees, float orbitDistance)
{
	setSphericalOnOffset(m_secondaryOrbitOffset, azimuthDegrees, elevationDegrees, orbitDistance);
	clampSecondaryOrbitOffset();
}

glm::vec3 ModelOrbitCamera::eyePosition() const
{
	return m_focus + m_orbitOffset;
}

void ModelOrbitCamera::setEyePosition(const glm::vec3& eye)
{
	m_orbitOffset = eye - m_focus;
	clampOrbitOffset();
}

glm::vec3 ModelOrbitCamera::secondaryEyePosition() const
{
	return m_focus + m_secondaryOrbitOffset;
}

void ModelOrbitCamera::setSecondaryEyePosition(const glm::vec3& eye)
{
	m_secondaryOrbitOffset = eye - m_focus;
	clampSecondaryOrbitOffset();
}

void ModelOrbitCamera::clampOrbitOffset()
{
	m_orbitOffset = clampOffsetLength(m_orbitOffset);
}

void ModelOrbitCamera::clampSecondaryOrbitOffset()
{
	m_secondaryOrbitOffset = clampOffsetLength(m_secondaryOrbitOffset);
}

void ModelOrbitCamera::updateFocus(const glm::vec3& center, float boundsRadius, bool updateCenter)
{
	if (updateCenter) {
		m_focus = center;
	}
	m_minDistance = std::max(boundsRadius * 0.5f, 0.5f);
	m_maxDistance = std::max(boundsRadius * 20.f, 100.f);

	const float currentDistance = glm::length(m_orbitOffset);
	if (!m_initialized || currentDistance < 0.001f) {
		const float defaultDistance = std::max(boundsRadius * 2.5f, 2.f);
		m_orbitOffset = glm::vec3{ 0.f, 0.f, defaultDistance };
		m_initialized = true;
	}
	else {
		clampOrbitOffset();
	}

	const float secondaryDistance = glm::length(m_secondaryOrbitOffset);
	if (!m_secondaryInitialized || secondaryDistance < 0.001f) {
		m_secondaryOrbitOffset = -m_orbitOffset;
		m_secondaryInitialized = true;
	}
	else {
		clampSecondaryOrbitOffset();
	}
}

void ModelOrbitCamera::applyView(const glm::vec3& eye)
{
	glLoadIdentity();
	gluLookAt(
		eye.x, eye.y, eye.z,
		m_focus.x, m_focus.y, m_focus.z,
		m_upVector.x, m_upVector.y, m_upVector.z);
}

void ModelOrbitCamera::setup()
{
	applyView(m_focus + m_orbitOffset);
}

void ModelOrbitCamera::setupSecondary()
{
	applyView(m_focus + m_secondaryOrbitOffset);
}

void ModelOrbitCamera::orbit(float change_x, float change_y, float factor)
{
	if (glm::length(m_orbitOffset) < 0.001f) {
		m_orbitOffset = glm::vec3{ 0.f, 0.f, m_minDistance };
	}

	glm::mat4x4 rotationMatrixX(1.f);
	rotationMatrixX = glm::rotate(rotationMatrixX, change_x * factor / 100.f, m_upVector);
	m_orbitOffset = glm::vec3(rotationMatrixX * glm::vec4(m_orbitOffset, 0.f));

	glm::vec3 right = glm::cross(m_orbitOffset, m_upVector);
	if (glm::length(right) < 1e-4f) {
		return;
	}
	right = glm::normalize(right);

	glm::mat4x4 rotationMatrixY(1.f);
	rotationMatrixY = glm::rotate(rotationMatrixY, change_y * factor / 100.f, right);
	m_orbitOffset = glm::vec3(rotationMatrixY * glm::vec4(m_orbitOffset, 0.f));
}

void ModelOrbitCamera::orbitSecondary(float change_x, float change_y, float factor)
{
	if (glm::length(m_secondaryOrbitOffset) < 0.001f) {
		m_secondaryOrbitOffset = glm::vec3{ 0.f, 0.f, -m_minDistance };
	}

	glm::mat4x4 rotationMatrixX(1.f);
	rotationMatrixX = glm::rotate(rotationMatrixX, change_x * factor / 100.f, m_upVector);
	m_secondaryOrbitOffset = glm::vec3(rotationMatrixX * glm::vec4(m_secondaryOrbitOffset, 0.f));

	glm::vec3 right = glm::cross(m_secondaryOrbitOffset, m_upVector);
	if (glm::length(right) < 1e-4f) {
		return;
	}
	right = glm::normalize(right);

	glm::mat4x4 rotationMatrixY(1.f);
	rotationMatrixY = glm::rotate(rotationMatrixY, change_y * factor / 100.f, right);
	m_secondaryOrbitOffset = glm::vec3(rotationMatrixY * glm::vec4(m_secondaryOrbitOffset, 0.f));
}

void ModelOrbitCamera::key(float change_x, float change_y, bool alternative, float factor)
{
	const float scale = 4.f;
	orbit(change_x * scale, change_y * scale, factor);
}

void ModelOrbitCamera::leftMouseStart()
{
}

void ModelOrbitCamera::leftMouse(float change_x, float change_y, float factor)
{
	orbit(change_x, change_y, factor);
}

void ModelOrbitCamera::leftMouseEnd()
{
}

void ModelOrbitCamera::rightMouseStart()
{
}

void ModelOrbitCamera::rightMouse(float change_x, float change_y, float factor)
{
	orbit(change_x, change_y, factor);
}

void ModelOrbitCamera::rightMouseEnd()
{
}

void ModelOrbitCamera::scroll(float scroll, float factor)
{
	const float len = glm::length(m_orbitOffset);
	if (len < 0.001f) {
		return;
	}

	float new_len = len + ((scroll * factor) / 2.f);
	new_len = std::clamp(new_len, m_minDistance, m_maxDistance);
	m_orbitOffset = glm::normalize(m_orbitOffset) * new_len;
}
