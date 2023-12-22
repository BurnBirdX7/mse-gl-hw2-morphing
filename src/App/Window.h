#pragma once

#include <functional>
#include <memory>

#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include "tiny_gltf.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Window final : public fgl::GLWidget
{
	Q_OBJECT
public:
	Window() noexcept;
	~Window() override;

public: // fgl::GLWidget
	void onInit() override;
	void onRender() override;
	void onResize(size_t width, size_t height) override;

public: // Controls
	void mousePressEvent(QMouseEvent* got_event) override;
	void mouseMoveEvent(QMouseEvent* got_event) override;
	void mouseReleaseEvent(QMouseEvent* got_event) override;
	void keyPressEvent(QKeyEvent* got_event) override;

private:
	class PerfomanceMetricsGuard final
	{
	public:
		explicit PerfomanceMetricsGuard(std::function<void()> callback);
		~PerfomanceMetricsGuard();

		PerfomanceMetricsGuard(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard(PerfomanceMetricsGuard &&) = delete;

		PerfomanceMetricsGuard & operator=(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard & operator=(PerfomanceMetricsGuard &&) = delete;

	private:
		std::function<void()> callback_;
	};

private:
	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();

	void load_model();

	void bind_model();
	void bind_buffers();
	void bind_textures();
	void bind_node(int nodeIdx);
	void bind_mesh(int meshIdx);

	void bind_vbo(int bufferViewIdx);

	void render();
	void render_model();
	void render_node(int nodeIdx);
	void render_mesh(int meshIdx);



signals:
	void updateUI();

private:
	struct {
		// Matrices
		GLint mvp = -1;
		GLint model = -1;
		GLint view = -1;
		GLint normal = -1;
	} uniforms_;

	QOpenGLVertexArrayObject vao_;

	glm::mat4 model_;
	glm::mat4 view_;
	glm::mat4 projection_;

	std::unique_ptr<QOpenGLTexture> texture_;
	std::unique_ptr<QOpenGLShaderProgram> program_;

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;

	// Controls tracking
	QPoint mouseTrackStart_;
	bool mouseTrack_ = false;

	// Render params
	struct Camera {
		glm::vec3 eye = {0, 2, 7};
		glm::vec3 up = {0, 1, 0};
		glm::vec3 front = {0, 0, 0};

		float yaw = -90.f;
		float pitch = -13.6;
		constexpr static float fow = 45.f;

		constexpr static float rotationSpeed = 0.05f;
		constexpr static float movementSpeed = 0.2f;

		Camera() {
			update_rotation(0, 0); // Force front vector update
		}

		inline void update_position(float deltaForward, float deltaRightward, float deltaUpward) {
			auto right = glm::normalize(glm::cross(front, up));
			eye += ((deltaForward * front) + (deltaRightward * right) + (deltaUpward * up)) * movementSpeed;
		}

		inline glm::vec3 get_target() const {
			return eye + front;
		}

		inline void update_rotation(float deltaYaw, float deltaPitch) {
			yaw += deltaYaw * rotationSpeed;
			pitch += deltaPitch * rotationSpeed;
			pitch = std::clamp(pitch, -89.f, +89.f); // Limit pitch to avoid lock

			front = glm::normalize(glm::vec3(
				cos(glm::radians(yaw) * cos(glm::radians(pitch))), // x = cos(yaw) * cos(pitch)
				sin(glm::radians(pitch)),									   // y = sin(pitch)
				sin(glm::radians(yaw) * cos(glm::radians(pitch)))  // z = sin(yaw) * cos(pitch)
			));
		}

		[[nodiscard]] inline glm::mat4 get_view() const {
			return glm::lookAt(eye, get_target(), up);
		}

	} camera_;

	// Model
	tinygltf::Model gltfModel_;
	std::vector<GLuint> vbos_; // Index is index of bufferView
};
