#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct AbstractCamera {
	virtual void updatePosition(float deltaForward, float deltaRightward, float deltaUpward) = 0;
	virtual void updateRotation(float deltaYaw, float deltaPitch) = 0;
	[[nodiscard]] virtual glm::mat4 getView() const = 0;
	[[nodiscard]] virtual QString getStats() const = 0;
};

struct FreeCamera : AbstractCamera {
	glm::vec3 eye_ = {7, 2, 0};
	glm::vec3 up_ = {0, 1, 0};
	glm::vec3 front_ = {0, 0, 0};

	float yaw_ = -180;
	float pitch_ = -14;

	bool relativeUp_ = false;

	constexpr static float ROTATION_SPEED = 0.05f;
	constexpr static float MOVEMENT_SPEED = 0.2f;

	FreeCamera()
	{
		updateRotation(0, 0);// Force front vector update
	}

	inline void updatePosition(float deltaForward, float deltaRightward, float deltaUpward) final
	{
		auto right = glm::normalize(glm::cross(front_, up_));
		glm::vec3 realUp = relativeUp_ ? glm::normalize(glm::cross(right, front_)) : up_;
		eye_ += ((deltaForward * front_) + (deltaRightward * right) + (deltaUpward * realUp)) * MOVEMENT_SPEED;
	}

	inline void updateRotation(float deltaYaw, float deltaPitch) final
	{
		yaw_ += deltaYaw * ROTATION_SPEED;
		pitch_ += deltaPitch * ROTATION_SPEED * 2;

		yaw_ = std::fmod(yaw_, 360.0f);
		pitch_ = std::clamp(pitch_, -85.f, +85.f);// Limit pitch_ to avoid lock

		float cosYaw = glm::cos(glm::radians(yaw_));
		float sinYaw = glm::sin(glm::radians(yaw_));
		float cosPitch = glm::cos(glm::radians(pitch_));
		float sinPitch = glm::sin(glm::radians(pitch_));

		front_ = glm::normalize(glm::vec3(
			cosYaw * cosPitch,
			sinPitch,
			sinYaw * cosPitch));
	}

	[[nodiscard]] inline glm::vec3 get_target() const
	{
		return eye_ + front_;
	}

	[[nodiscard]] inline glm::mat4 getView() const override
	{
		return glm::lookAt(eye_, get_target(), up_);
	}

	[[nodiscard]] QString getStats() const override
	{
		return QString("Camera | position: (%1, %2, %3), yaw_: %4, pitch_: %5")
			.arg(eye_.x)
			.arg(eye_.y)
			.arg(eye_.z)
			.arg(yaw_)
			.arg(pitch_);
	}
};

struct RotatingCamera : AbstractCamera {
	float radius_ = 6.0f;
	float theta_ = 75.0f;
	float phi_ = 0.0f;
	glm::vec3 eye_ = {};

	RotatingCamera()
	{
		updatePosition(0, 0, 0);
	}

	constexpr static float ROTATION_SPEED = 1.0f;
	constexpr static float MOVEMENT_SPEED = 0.2f;

	inline void updateRotation(float, float) final
	{
		// Ignore
	}

	inline void updatePosition(float deltaForward, float deltaRightward, float deltaUpward) final
	{
		radius_ += deltaUpward * MOVEMENT_SPEED;
		phi_ -= deltaRightward * ROTATION_SPEED;
		theta_ -= deltaForward * ROTATION_SPEED;

		phi_ = std::fmod(phi_, 360.0f);
		theta_ = std::clamp(theta_, 0.001f, 179.999f);

		eye_ = radius_ * glm::vec3(glm::sin(glm::radians(theta_)) * glm::cos(glm::radians(phi_)), glm::cos(glm::radians(theta_)), glm::sin(glm::radians(theta_)) * glm::sin(glm::radians(phi_)));
	}

	[[nodiscard]] inline glm::mat4 getView() const override
	{
		return glm::lookAt(eye_, -glm::normalize(eye_), glm::vec3(0, 1, 0));
	}

	[[nodiscard]] QString getStats() const override
	{
		return QString("Camera | position: (%1, %2, %3), phi_: %4, theta_: %5")
			.arg(eye_.x)
			.arg(eye_.y)
			.arg(eye_.z)
			.arg(phi_)
			.arg(theta_);
	}
};