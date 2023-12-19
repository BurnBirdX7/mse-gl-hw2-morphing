#include "Window.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <array>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

namespace
{

constexpr std::array<GLfloat, 21u> vertices = {
	0.0f, 0.707f, 1.f, 0.f, 0.f, 0.0f, 0.0f,
	-0.5f, -0.5f, 0.f, 1.f, 0.f, 0.5f, 1.0f,
	0.5f, -0.5f, 0.f, 0.f, 1.f, 1.0f, 0.0f,
};
constexpr std::array<GLuint, 3u> indices = {0, 1, 2};

}// namespace

Window::Window() noexcept
{
	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");

	auto layout = new QVBoxLayout();
	layout->addWidget(fps, 1);

	setLayout(layout);

	timer_.start();

	connect(this, &Window::updateUI, [=] {
		fps->setText(formatFPS(ui_.fps));
	});
}

Window::~Window()
{
	{
		// Free resources with context bounded.
		const auto guard = bindContext();
		texture_.reset();
		program_.reset();
	}
}

void Window::onInit()
{
	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/diffuse.vs");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
									  ":/Shaders/diffuse.fs");
	program_->link();

	// Create VAO object
	vao_.create();
	vao_.bind();

	// Load and bind model
	load_model();
	bind_model();

	texture_ = std::make_unique<QOpenGLTexture>(QImage(":/Textures/voronoi.png"));
	texture_->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
	texture_->setWrapMode(QOpenGLTexture::WrapMode::Repeat);

	// Bind attributes
	program_->bind();

	program_->enableAttributeArray(0);
	program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, static_cast<int>(7 * sizeof(GLfloat)));

	program_->enableAttributeArray(1);
	program_->setAttributeBuffer(1, GL_FLOAT, static_cast<int>(2 * sizeof(GLfloat)), 3,
								 static_cast<int>(7 * sizeof(GLfloat)));

	program_->enableAttributeArray(2);
	program_->setAttributeBuffer(2, GL_FLOAT, static_cast<int>(5 * sizeof(GLfloat)), 2,
								 static_cast<int>(7 * sizeof(GLfloat)));

	mvpUniform_ = program_->uniformLocation("mvp");

	// Release all
	program_->release();

	vao_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate MVP matrix
	model_.setToIdentity();
	model_.translate(0, 0, -2);
	view_.setToIdentity();
	const auto mvp = projection_ * view_ * model_;

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	// Update uniform value
	program_->setUniformValue(mvpUniform_, mvp);

	// Activate texture unit and bind texture
	glActiveTexture(GL_TEXTURE0);
	texture_->bind();

	// Draw
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

	// Release VAO and shader program
	texture_->release();
	vao_.release();
	program_->release();

	++frameCount_;

	// Request redraw if animated
	if (animated_)
	{
		update();
	}
}

void Window::onResize(const size_t width, const size_t height)
{
	// Configure viewport
	glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

	// Configure matrix
	const auto aspect = static_cast<float>(width) / static_cast<float>(height);
	const auto zNear = 0.1f;
	const auto zFar = 100.0f;
	const auto fov = 60.0f;
	projection_.setToIdentity();
	projection_.perspective(fov, aspect, zNear, zFar);
}

Window::PerfomanceMetricsGuard::PerfomanceMetricsGuard(std::function<void()> callback)
	: callback_{ std::move(callback) }
{
}

Window::PerfomanceMetricsGuard::~PerfomanceMetricsGuard()
{
	if (callback_)
	{
		callback_();
	}
}

auto Window::captureMetrics() -> PerfomanceMetricsGuard
{
	return PerfomanceMetricsGuard{
		[&] {
			if (timer_.elapsed() >= 1000)
			{
				const auto elapsedSeconds = static_cast<float>(timer_.restart()) / 1000.0f;
				ui_.fps = static_cast<size_t>(std::round(frameCount_ / elapsedSeconds));
				frameCount_ = 0;
				emit updateUI();
			}
		}
	};
}
void Window::load_model()
{
	auto ctx = tinygltf::TinyGLTF();

	std::string err;
	std::string warn;

	auto file = QFile(":Models/Cube.glb");
	auto qBytes = file.readAll();
	auto bytes = reinterpret_cast<const unsigned char*>(qBytes.constData());
	auto len = qBytes.length();

	bool res = ctx.LoadBinaryFromMemory(&gltf_model_, &err, &warn, bytes, len);

	if (!res) {
		qDebug() << "Couldn't load the model";
		qDebug() << "WARN: " << QString::fromStdString(warn);
		qDebug() << "ERR:  " << QString::fromStdString(err);
		throw std::runtime_error("Cannot load model");
	}

	qDebug() << "Loaded GLTF";
}

void Window::bind_mesh(int meshIdx)
{
	auto& mesh = gltf_model_.meshes[meshIdx];
	for (auto& primitive: mesh.primitives) {
		for (auto const& [name, accessorIdx]: primitive.attributes) {

			auto accessor = gltf_model_.accessors[accessorIdx];
			auto bufferViewIdx = accessor.bufferView;
			auto& vbo = vbos_[bufferViewIdx];

			int vaa;
			if (name == "POSITION") vaa = 0;
			else if (name == "NORMAL") vaa = 1;
			else if (name == "TEXCOORD_0") vaa = 2;
			else {
				qDebug() << "Attribute \"" << QString::fromStdString(name) << "\" was skipped";
				continue;
			}

			int size = 1;
			if (accessor.type == TINYGLTF_TYPE_SCALAR) {
				size = 1;
			} else if (accessor.type == TINYGLTF_TYPE_VEC2) {
				size = 2;
			} else if (accessor.type == TINYGLTF_TYPE_VEC3) {
				size = 3;
			} else if (accessor.type == TINYGLTF_TYPE_VEC4) {
				size = 4;
			} else {
				qDebug() << "Unsupported accessor type: " << accessor.type;
				throw std::runtime_error("Unsupported accessor type");
			}

			auto& bufferView = gltf_model_.bufferViews[bufferViewIdx];
			int byteStride = accessor.ByteStride(bufferView);

			glEnableVertexAttribArray(vaa);
			glVertexAttribPointer(vaa, size,
								  accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE,
								  byteStride, reinterpret_cast<const void *>(bufferView.byteOffset));

		}

	}
}

void Window::bind_node(int nodeIdx)
{
	auto& node = gltf_model_.nodes[nodeIdx];
	if ((node.mesh >= 0) && (node.mesh <= gltf_model_.meshes.size())) {
		bind_mesh(node.mesh);
	}

	for (auto& childIdx: node.children) {
		bind_node(childIdx);
	}
}

void Window::bind_model()
{
	auto scene = gltf_model_.scenes[gltf_model_.defaultScene];
	vbos_.resize(gltf_model_.bufferViews.size());

	// Generate VBOs
	glGenBuffers(vbos_.size(), vbos_.data());
	for (size_t i = 0; i < vbos_.size(); ++i) {
		auto bufferView = gltf_model_.bufferViews[i];
		auto vbo = vbos_[i];

		if (bufferView.target == 0) {
			continue;
		}

		auto& buffer = gltf_model_.buffers[bufferView.buffer];
		glBindBuffer(bufferView.target, vbo);
		glBufferData(bufferView.target,
					 bufferView.byteLength,
					 buffer.data.data() + bufferView.byteOffset,
					 GL_STATIC_DRAW);

	}

	// Bind model's nodes
	for (auto nodeIdx: scene.nodes) {
		bind_node(nodeIdx);
	}

}

GLuint Window::generate_and_bind_vbo(int bufferViewIdx)
{
	auto& bufferView = gltf_model_.bufferViews[bufferViewIdx];
	auto& buffer = gltf_model_.buffers[bufferView.buffer];

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(bufferView.target, vbo);
	glBufferData(bufferView.target,
				 bufferView.byteLength,
				 buffer.data.data() + bufferView.byteOffset,
				 GL_STATIC_DRAW);

	return vbo;
}
void Window::bind_vbo(int bufferViewIdx, GLuint vbo)
{
	auto& bufferView = gltf_model_.bufferViews[bufferViewIdx];
	glBindBuffer(bufferView.target, vbo);
}

void Window::render_model()
{
	const auto& scene = gltf_model_.scenes[gltf_model_.defaultScene];
	for (auto& nodeIdx: scene.nodes) {
		render_node(nodeIdx);
	}
}
void Window::render_node(int nodeIdx)
{
	auto& node = gltf_model_.nodes[nodeIdx];
	if ((node.mesh >= 0) && (node.mesh <= gltf_model_.meshes.size())) {
		render_mesh(node.mesh);
	}

	for (auto& childIdx: node.children) {
		render_node(childIdx);
	}
}
void Window::render_mesh(int meshIdx)
{
	auto& mesh = gltf_model_.meshes[meshIdx];
	for (auto& primitive: mesh.primitives) {
		auto& accessor = gltf_model_.accessors[primitive.indices];
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[accessor.bufferView]);
		glDrawElements(primitive.mode,
					   accessor.count,
					   accessor.componentType,
					   reinterpret_cast<void*>(accessor.byteOffset));
	}
}
