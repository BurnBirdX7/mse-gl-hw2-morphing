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
#include <QLabel>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tiny_gltf.h"

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

signals:
	void updateUI();

public slots:
	void changeCameraType(bool);
	void switchDiffuseLight(bool);
	void switchSpotLight(bool);
	void morph(int);
	void relativeUp(bool);

private:
	class PerfomanceMetricsGuard final {
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

	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();

private:
	/*  ~ GLTF Model Functions ~  */
	// Load

	void gltfLoadModel();

	// Bind

	void gltfBindModel();
	void gltfBindBuffers();
	void gltfBindTextures();
	void gltfBindNode(int nodeIdx);
	void gltfBindMesh(int meshIdx);

	// Render

	void gltfRenderModel();
	void gltfRenderNode(int nodeIdx);
	void gltfRenderMesh(int meshIdx);

	void render();

private:
	/*  ~ OpenGL Data ~  */
	struct {
		GLint mvp = -1;
		GLint model = -1;
		GLint view = -1;
		GLint normal = -1;
		GLint morph = -1;
		GLint enableDiffuse = -1;
		GLint enableSpot = -1;
	} uniforms_;

	std::unique_ptr<QOpenGLShaderProgram> program_;
	QOpenGLVertexArrayObject vao_;


	/*  ~ Render Data ~  */

	// Transformations
	glm::mat4 model_;
	glm::mat4 view_;
	glm::mat4 projection_;

	// Cameras
	FreeCamera freeCamera_{};
	RotatingCamera rotatingCamera_{};
	AbstractCamera* currentCamera_ = &rotatingCamera_;
	QLabel* cameraStats_;

	// Model
	tinygltf::Model gltfModel_;
	std::vector<GLuint> vbos_; // Index is index of bufferView
	std::vector<GLuint> textures_; // Index is index of texture

	// Uniform values
	float morph_ = 0;
	bool enableDiffuse_ = false;
	bool enableSpot_ = false;
	bool relativeUp_ = false;


	/*  ~ Stats and Behaviour Tracking ~  */
	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;

	// Controls tracking
	QPoint mouseTrackStart_;
	bool mouseTrack_ = false;

};
