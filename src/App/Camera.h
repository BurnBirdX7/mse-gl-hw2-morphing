#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct AbstractCamera {
	virtual void update_position(float deltaForward, float deltaRightward, float deltaUpward) = 0;
	virtual void update_rotation(float deltaYaw, float deltaPitch) = 0;
	[[nodiscard]] virtual glm::mat4 get_view() const = 0;
};

struct FreeCamera : AbstractCamera {
	glm::vec3 eye = {7, 2, 0};
	glm::vec3 up = {0, 1, 0};
	glm::vec3 front = {0, 0, 0};

	float yaw = -180;
	float pitch = -14;
	constexpr static float rotationSpeed = 0.05f;
	constexpr static float movementSpeed = 0.2f;

	FreeCamera() {
		update_rotation(0, 0); // Force front vector update
	}

	inline void update_position(float deltaForward, float deltaRightward, float deltaUpward) final {
		auto right = glm::normalize(glm::cross(front, up));
		eye += ((deltaForward * front) + (deltaRightward * right) + (deltaUpward * up)) * movementSpeed;
	}

	inline void update_rotation(float deltaYaw, float deltaPitch) final {
		yaw += deltaYaw * rotationSpeed;
		pitch += deltaPitch * rotationSpeed * 2;

		yaw = std::fmod(yaw, 360.0f);
		pitch = std::clamp(pitch, -89.f, +89.f); // Limit pitch to avoid lock

		front = glm::normalize(glm::vec3(
			cos(glm::radians(yaw) * cos(glm::radians(pitch))), // x = cos(yaw) * cos(pitch)
			sin(glm::radians(pitch)),									   // y = sin(pitch)
			sin(glm::radians(yaw) * cos(glm::radians(pitch)))  // z = sin(yaw) * cos(pitch)
			));
	}

	[[nodiscard]] inline glm::vec3 get_target() const {
		return eye + front;
	}

	[[nodiscard]] inline glm::mat4 get_view() const {
		return glm::lookAt(eye, get_target(), up);
	}

};

struct RotatingCamera : AbstractCamera {
	float radius = 6.0f;
	float theta = 75.0f;
	float phi = 0.0f;
	glm::vec3 eye{};

	RotatingCamera() {
		update_position(0, 0, 0);
	}

	constexpr static float rotationSpeed = 1.0f;
	constexpr static float movementSpeed = 0.2f;

	inline void update_rotation(float, float) final {
		// Ignore
	}

	inline void update_position(float deltaForward, float deltaRightward, float deltaUpward) final {
		radius += deltaUpward * movementSpeed;
		phi -= deltaRightward * rotationSpeed;
		theta -= deltaForward * rotationSpeed;

		phi = std::fmod(phi, 360.0f);
		theta = std::clamp(theta, 0.001f, 179.999f);

		eye = radius * glm::vec3(
				  glm::sin(glm::radians(theta)) * glm::cos(glm::radians(phi)),
				  glm::cos(glm::radians(theta)),
				  glm::sin(glm::radians(theta)) * glm::sin(glm::radians(phi))
			  );
	}


	[[nodiscard]] inline glm::mat4 get_view() const {
		return glm::lookAt(eye, -glm::normalize(eye), glm::vec3(0, 1, 0));
	}

};