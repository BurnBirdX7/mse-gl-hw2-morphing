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

#include "Camera.h"


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

public slots:
	void changeCameraType(bool);
	void switchDiffuseLight(bool);
	void switchSpotLight(bool);
	void morph(int);

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
		GLint morph = -1;
		GLint enableDiffuse = -1;
		GLint enableSpot = -1;
	} uniforms_;

	QOpenGLVertexArrayObject vao_;

	glm::mat4 model_;
	glm::mat4 view_;
	glm::mat4 projection_;

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

	// Cameras
	FreeCamera freeCamera_{};
	RotatingCamera rotatingCamera_{};
	AbstractCamera* currentCamera_ = &rotatingCamera_;

	// Model
	tinygltf::Model gltfModel_;
	std::vector<GLuint> vbos_; // Index is index of bufferView
	std::vector<GLuint> textures_; // Index is index of texture

	// State control
	float morph_ = 0;
	bool enableDiffuse_ = false;
	bool enableSpot_ = false;

};
