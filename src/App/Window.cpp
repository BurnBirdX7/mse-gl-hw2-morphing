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

const QString MODEL_TO_LOAD = ":Models/Cube.glb";


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
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/vertex.glsl");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/fragment.glsl");
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

	uniforms_.mvp = program_->uniformLocation("mvp");
	uniforms_.model = program_->uniformLocation("modelMat");
	uniforms_.view = program_->uniformLocation("viewMat");
	uniforms_.normal = program_->uniformLocation("normalMat");

	// Release all
	program_->release();

	vao_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	model_ = glm::rotate(glm::mat4(1), glm::radians(45.f), glm::vec3(0, 1, 0));
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	// Clear screen
	glClearColor(0.2, 0.2, 0.2, 1.0);		// background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activate texture unit and bind texture
	glActiveTexture(GL_TEXTURE0);
	texture_->bind();

	// Render
	render();

	// Release VAO and shader program
	texture_->release();
	vao_.release();
	program_->release();

	++frameCount_;

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		qDebug() << "OpenGL Error: " << error;
	}

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
	const auto fov = Camera::fow;
	projection_ = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
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

	auto file = QFile(MODEL_TO_LOAD);

	if (!file.open(QIODevice::ReadOnly)) {
		qDebug() << "Cannot open file " << file.fileName();
		throw std::runtime_error("Cannot open model file");
	}

	auto qBytes = file.readAll();
	auto bytes = reinterpret_cast<const unsigned char*>(qBytes.constData());
	auto len = qBytes.length();

	bool res = ctx.LoadBinaryFromMemory(&gltfModel_, &err, &warn, bytes, len);

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
	auto scene = gltfModel_.scenes[gltfModel_.defaultScene];
	vbos_.resize(gltfModel_.bufferViews.size());

	bind_buffers();
	bind_textures();

	// Bind model's nodes
	for (auto nodeIdx: scene.nodes) {
		bind_node(nodeIdx);
	}

}

void Window::bind_buffers() {
	// Generate VBOs and bind buffer views to them
	for (size_t i = 0; i < gltfModel_.bufferViews.size(); ++i) {
		auto& bufferView = gltfModel_.bufferViews[i];

		auto qname = QString::fromStdString(bufferView.name);
		qDebug() << "Binding buffer" << i << qname;

		if (bufferView.target == 0) {
			qDebug() << "Unsupported target at buffer" << i << qname;
			continue;
		}

		auto& vbo = vbos_[i];  // There's already place for the VBO in the container
		glGenBuffers(1, &vbo);   // Generate buffer, and store its ID in the container
		qDebug() << "Generated buffer for bufferView" << i;

		// Associate data with the buffer:
		auto& buffer = gltfModel_.buffers[bufferView.buffer];
		glBindBuffer(bufferView.target, vbo);
		glBufferData(bufferView.target,  // Set data
					 bufferView.byteLength,
					 buffer.data.data() + bufferView.byteOffset,
					 GL_STATIC_DRAW);

	}
}

void Window::bind_textures()
{
	// TODO: Bind textures
}

void Window::bind_node(int nodeIdx)
{
	auto& node = gltfModel_.nodes[nodeIdx];
	auto qname = QString::fromStdString(node.name);
	qDebug() << "Binding node" << nodeIdx << qname;
	if ((node.mesh >= 0) && ((size_t)node.mesh < gltfModel_.meshes.size())) {
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
	auto& mesh = gltfModel_.meshes[meshIdx];
	qDebug() << "Mesh" << meshIdx << QString::fromStdString(mesh.name);
	for (auto& primitive: mesh.primitives) {
		for (auto const& [name, accessorIdx]: primitive.attributes) {
			auto accessor = gltfModel_.accessors[accessorIdx];
			auto bufferViewIdx = accessor.bufferView;
			glBindBuffer(GL_ARRAY_BUFFER, vbos_[bufferViewIdx]);

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

			auto& bufferView = gltfModel_.bufferViews[bufferViewIdx];
			int byteStride = accessor.ByteStride(bufferView);

			glEnableVertexAttribArray(vaa);
			glVertexAttribPointer(vaa, size,
								  accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE,
								  byteStride, BUFFER_OFFSET(accessor.byteOffset));

			qDebug() << "Bound mesh" << meshIdx << "vaa" << vaa << "size" << size
					 << "bufferView" << bufferViewIdx << "Offset" << bufferView.byteOffset
					 << "Stride" << byteStride << "Component type" << accessor.componentType;
		}

	}
}

void Window::render_model()
{
	const auto& scene = gltfModel_.scenes[gltfModel_.defaultScene];
	for (auto& nodeIdx: scene.nodes) {
		render_node(nodeIdx);
	}
}
void Window::render_node(int nodeIdx)
{
	auto& node = gltfModel_.nodes[nodeIdx];
	if ((node.mesh >= 0) && (node.mesh <= gltfModel_.meshes.size())) {
		render_mesh(node.mesh);
	}

	for (auto& childIdx: node.children) {
		render_node(childIdx);
	}
}
void Window::render_mesh(int meshIdx)
{
	auto& mesh = gltfModel_.meshes[meshIdx];
	for (auto& primitive: mesh.primitives) {
		auto& accessor = gltfModel_.accessors[primitive.indices];
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[accessor.bufferView]);
		glDrawElements(primitive.mode,
					   accessor.count,
					   accessor.componentType,
					   BUFFER_OFFSET(accessor.byteOffset));
	}
}
void Window::mousePressEvent(QMouseEvent* got_event)
{
	mouseTrack_ = true;
	mouseTrackStart_ = got_event->pos();
}
void Window::mouseMoveEvent(QMouseEvent* got_event)
{
	if (!mouseTrack_) {
		return;
	}

	auto pos = got_event->pos();
	auto deltaX = mouseTrackStart_.x() - pos.x();
	auto deltaY = pos.y() - mouseTrackStart_.y(); // Inverted Y
	mouseTrackStart_ = pos;
	camera_.update_rotation(static_cast<float>(deltaX), static_cast<float>(deltaY));
	update();

}
void Window::mouseReleaseEvent(QMouseEvent* got_event)
{
	mouseTrack_ = false;
}

void Window::keyPressEvent(QKeyEvent * got_event) {
	auto key = got_event->key();

	static std::map<Qt::Key, glm::vec3> keymap = {
		{Qt::Key_W, {1, 0, 0}},
		{Qt::Key_S, {-1, 0, 0}},
		{Qt::Key_A, {0, 0, -1}},
		{Qt::Key_D, {0, 0, 1}},
		{Qt::Key_Space, {0, 1, 0}},
		{Qt::Key_Control, {0, -1, 0}},
	};

	auto delta = keymap[(Qt::Key)key];
	camera_.update_position(delta.x, delta.z, delta.y);

	update();
}
void Window::render()
{
	// Update view matrix
	view_ = camera_.get_view();
	const auto mvp = projection_ * view_ * model_;

	// Update model matrix -- skip
	// Update normal matrix:
	auto normalMat = glm::inverse(model_);

	// Set uniforms:
	program_->setUniformValue(uniforms_.model, QMatrix4x4(glm::value_ptr(model_)).transposed());
	program_->setUniformValue(uniforms_.view, QMatrix4x4(glm::value_ptr(view_)).transposed());
	program_->setUniformValue(uniforms_.normal, QMatrix4x4(glm::value_ptr(normalMat)));
	program_->setUniformValue(uniforms_.mvp, QMatrix4x4(glm::value_ptr(mvp)).transposed());

	// Render model:
	render_model();
}
