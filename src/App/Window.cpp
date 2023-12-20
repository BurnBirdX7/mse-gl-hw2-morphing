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

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


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

	// Load texture
	texture_ = std::make_unique<QOpenGLTexture>(QImage(":/Textures/oxy.png"));
	texture_->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
	texture_->setWrapMode(QOpenGLTexture::WrapMode::Repeat);

	// Bind attributes
	program_->bind();

	mvpUniform_ = program_->uniformLocation("mvp");

	// Release all
	program_->release();

	vao_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cameraPos_ = glm::vec3(0, 7, 7);
	cameraFront_ = glm::vec3(0, 0, -4);
	cameraUp_ = glm::vec3(0, 1, 0);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate MVP matrix
	model_ = glm::mat4(1);
	view_ = glm::lookAt(cameraPos_, cameraPos_ + cameraFront_, cameraUp_);
	const auto mvp = projection_ * view_ * model_;

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	// Update uniform value
	program_->setUniformValue(mvpUniform_, QMatrix4x4(glm::value_ptr(mvp)).transposed());

	// Activate texture unit and bind texture
	glActiveTexture(GL_TEXTURE0);
	texture_->bind();

	// Draw
	render_model();

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
	projection_ = glm::mat4(1.0f);
	projection_ = glm::perspective(fov, aspect, zNear, zFar);
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

	if (!file.open(QIODevice::ReadOnly)) {
		qDebug() << "Cannot open file " << file.fileName();
		throw std::runtime_error("Cannot open model file");
	}

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

void Window::bind_model()
{
	auto scene = gltf_model_.scenes[gltf_model_.defaultScene];
	vbos_.resize(gltf_model_.bufferViews.size());

	// Generate VBOs
	for (size_t i = 0; i < vbos_.size(); ++i) {
		auto& bufferView = gltf_model_.bufferViews[i];
		auto& vbo = vbos_[i];

		if (bufferView.target == 0) {
			qDebug() << "Unsupported target at buffer" << i;
			continue;
		}


		glGenBuffers(1, &vbo);
		qDebug() << "Generated buffer for bufferView" << i;

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

void Window::bind_node(int nodeIdx)
{
	qDebug() << "Binding node" << nodeIdx;

	auto& node = gltf_model_.nodes[nodeIdx];
	if ((node.mesh >= 0) && (node.mesh <= gltf_model_.meshes.size())) {
		qDebug() << "Node" << nodeIdx << " -> Mesh" << node.mesh;
		bind_mesh(node.mesh);
	} else {
		qDebug() << "Node" << nodeIdx << "has no valid mesh";
	}

	for (auto& childIdx: node.children) {
		qDebug() << "Node" << childIdx << " is a child of Node" << nodeIdx;
		bind_node(childIdx);
	}
}

void Window::bind_mesh(int meshIdx)
{
	auto& mesh = gltf_model_.meshes[meshIdx];
	for (auto& primitive: mesh.primitives) {
		for (auto const& [name, accessorIdx]: primitive.attributes) {

			auto accessor = gltf_model_.accessors[accessorIdx];
			auto bufferViewIdx = accessor.bufferView;
			bind_vbo(bufferViewIdx);

			int vaa;
			if (name == "POSITION") vaa = 0;
			else if (name == "NORMAL") vaa = 1;
			else if (name == "TEXCOORD_0") vaa = 2;
			else {
				qDebug() << "Attribute" << QString::fromStdString(name) << "was skipped in mesh" << meshIdx;
				continue;
			}

			int size;
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
								  byteStride, BUFFER_OFFSET(bufferView.byteOffset));

		}

	}
}

void Window::bind_vbo(int bufferViewIdx)
{
	auto& bufferView = gltf_model_.bufferViews[bufferViewIdx];
	auto vbo = vbos_[bufferViewIdx];
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
					   BUFFER_OFFSET(accessor.byteOffset));
	}
}
