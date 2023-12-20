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
	void bind_node(int nodeIdx);
	void bind_mesh(int meshIdx);

	GLuint generate_and_bind_vbo(int bufferViewIdx);
	void bind_vbo(int bufferViewIdx);

	void render_model();
	void render_node(int nodeIdx);
	void render_mesh(int meshIdx);


signals:
	void updateUI();

private:
	GLint mvpUniform_ = -1;

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

	// Render params
	glm::vec3 cameraPos_;
	glm::vec3 cameraFront_;
	glm::vec3 cameraUp_;

	// Model
	tinygltf::Model gltf_model_;
	std::vector<GLuint> vbos_; // Index is index of bufferView
};
