#include "stdafx.h"
#include "CameraControl.h"
#include "RenderWidget.h"
#include "ModelOrbitCamera.h"
#include <glm/glm.hpp>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QScrollArea>

CameraControl::CameraControl(QWidget* parent)
	: QWidget(parent),
	renderWidget(nullptr),
	isLoadingUi(false),
	isOrbitCamera(false)
{
	buildUi();
	bindControls();
	setActive(false);
}

CameraControl::~CameraControl()
{}

void CameraControl::setRenderWidget(RenderWidget* widget)
{
	if (renderWidget != nullptr) {
		disconnect(renderWidget, nullptr, this, nullptr);
	}

	renderWidget = widget;

	if (renderWidget != nullptr) {
		connect(renderWidget, &RenderWidget::cameraChanged, this, &CameraControl::syncFromCamera);
		syncFromCamera();
	}
	else {
		setActive(false);
	}
}

CameraControl::AxisRow CameraControl::addAxisRow(QFormLayout* layout, const QString& label, float min, float max, float step, float sliderScale)
{
	AxisRow row;
	row.sliderScale = sliderScale;

	auto* container = new QWidget(this);
	auto* rowLayout = new QHBoxLayout(container);
	rowLayout->setContentsMargins(0, 0, 0, 0);

	row.slider = new QSlider(Qt::Horizontal, container);
	row.slider->setMinimum(static_cast<int>(min * sliderScale));
	row.slider->setMaximum(static_cast<int>(max * sliderScale));
	row.slider->setSingleStep(static_cast<int>(step * sliderScale));

	row.spin = new QDoubleSpinBox(container);
	row.spin->setRange(min, max);
	row.spin->setSingleStep(step);
	row.spin->setDecimals(3);

	rowLayout->addWidget(row.slider, 3);
	rowLayout->addWidget(row.spin, 1);

	layout->addRow(label, container);
	return row;
}

void CameraControl::buildUi()
{
	auto* outerLayout = new QVBoxLayout(this);
	outerLayout->setContentsMargins(0, 0, 0, 0);

	auto* scrollArea = new QScrollArea(this);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setWidgetResizable(true);

	auto* contents = new QWidget(scrollArea);
	auto* layout = new QVBoxLayout(contents);

	auto* optionsGroup = new QGroupBox(QStringLiteral("Primary camera (left view)"), contents);
	auto* optionsLayout = new QVBoxLayout(optionsGroup);
	checkBoxFollowModel = new QCheckBox(QStringLiteral("Follow model center"), optionsGroup);
	checkBoxFollowModel->setChecked(true);
	pushButtonReset = new QPushButton(QStringLiteral("Reset camera"), optionsGroup);
	optionsLayout->addWidget(checkBoxFollowModel);
	optionsLayout->addWidget(pushButtonReset);
	layout->addWidget(optionsGroup);

	auto* focusGroup = new QGroupBox(QStringLiteral("Focus point"), contents);
	auto* focusLayout = new QFormLayout(focusGroup);
	focusX = addAxisRow(focusLayout, QStringLiteral("X"), -100.f, 100.f, 0.01f, 100.f);
	focusY = addAxisRow(focusLayout, QStringLiteral("Y"), -100.f, 100.f, 0.01f, 100.f);
	focusZ = addAxisRow(focusLayout, QStringLiteral("Z"), -100.f, 100.f, 0.01f, 100.f);
	layout->addWidget(focusGroup);

	auto* offsetGroup = new QGroupBox(QStringLiteral("Orbit offset"), contents);
	auto* offsetLayout = new QFormLayout(offsetGroup);
	offsetX = addAxisRow(offsetLayout, QStringLiteral("X"), -100.f, 100.f, 0.01f, 100.f);
	offsetY = addAxisRow(offsetLayout, QStringLiteral("Y"), -100.f, 100.f, 0.01f, 100.f);
	offsetZ = addAxisRow(offsetLayout, QStringLiteral("Z"), -100.f, 100.f, 0.01f, 100.f);
	layout->addWidget(offsetGroup);

	auto* sphericalGroup = new QGroupBox(QStringLiteral("Orbit (spherical)"), contents);
	auto* sphericalLayout = new QFormLayout(sphericalGroup);
	distance = addAxisRow(sphericalLayout, QStringLiteral("Distance"), 0.f, 200.f, 0.01f, 100.f);
	azimuth = addAxisRow(sphericalLayout, QStringLiteral("Azimuth (deg)"), 0.f, 360.f, 0.1f, 10.f);
	elevation = addAxisRow(sphericalLayout, QStringLiteral("Elevation (deg)"), -90.f, 90.f, 0.1f, 10.f);
	layout->addWidget(sphericalGroup);

	auto* eyeGroup = new QGroupBox(QStringLiteral("Eye position"), contents);
	auto* eyeLayout = new QFormLayout(eyeGroup);
	eyeX = addAxisRow(eyeLayout, QStringLiteral("X"), -100.f, 100.f, 0.01f, 100.f);
	eyeY = addAxisRow(eyeLayout, QStringLiteral("Y"), -100.f, 100.f, 0.01f, 100.f);
	eyeZ = addAxisRow(eyeLayout, QStringLiteral("Z"), -100.f, 100.f, 0.01f, 100.f);
	layout->addWidget(eyeGroup);

	auto* upGroup = new QGroupBox(QStringLiteral("Up vector"), contents);
	auto* upLayout = new QFormLayout(upGroup);
	upX = addAxisRow(upLayout, QStringLiteral("X"), -1.f, 1.f, 0.01f, 1000.f);
	upY = addAxisRow(upLayout, QStringLiteral("Y"), -1.f, 1.f, 0.01f, 1000.f);
	upZ = addAxisRow(upLayout, QStringLiteral("Z"), -1.f, 1.f, 0.01f, 1000.f);
	layout->addWidget(upGroup);

	auto* limitsGroup = new QGroupBox(QStringLiteral("Zoom limits"), contents);
	auto* limitsLayout = new QFormLayout(limitsGroup);
	minDistance = addAxisRow(limitsLayout, QStringLiteral("Min distance"), 0.01f, 200.f, 0.01f, 100.f);
	maxDistance = addAxisRow(limitsLayout, QStringLiteral("Max distance"), 0.01f, 500.f, 0.1f, 10.f);
	layout->addWidget(limitsGroup);

	layout->addStretch(1);
	scrollArea->setWidget(contents);
	outerLayout->addWidget(scrollArea);
}

void CameraControl::bindControls()
{
	auto bindAxis = [&](AxisRow& row) {
		connect(row.slider, &QSlider::valueChanged, this, [&, rowPtr = &row](int value) {
			if (isLoadingUi || rowPtr->setter == nullptr) {
				return;
			}
			rowPtr->setter(static_cast<float>(value) / rowPtr->sliderScale);
			notifyCameraEdited();
		});

		connect(row.spin, &QDoubleSpinBox::valueChanged, this, [&, rowPtr = &row](double value) {
			if (isLoadingUi || rowPtr->setter == nullptr) {
				return;
			}
			rowPtr->setter(static_cast<float>(value));
			notifyCameraEdited();
		});
	};

	for (auto* row : {
		&focusX, &focusY, &focusZ,
		&offsetX, &offsetY, &offsetZ,
		&distance, &azimuth, &elevation,
		&eyeX, &eyeY, &eyeZ,
		&upX, &upY, &upZ,
		&minDistance, &maxDistance
	}) {
		bindAxis(*row);
	}

	connect(checkBoxFollowModel, &QCheckBox::toggled, this, [this](bool checked) {
		if (renderWidget != nullptr) {
			renderWidget->setFollowsModelFocus(checked);
			notifyCameraEdited();
		}
	});

	connect(pushButtonReset, &QPushButton::clicked, this, [this]() {
		if (renderWidget != nullptr) {
			renderWidget->resetCamera();
		}
	});
}

void CameraControl::setActive(bool active)
{
	isOrbitCamera = active;
	checkBoxFollowModel->setEnabled(active);
	pushButtonReset->setEnabled(active);

	for (auto* row : {
		&focusX, &focusY, &focusZ,
		&offsetX, &offsetY, &offsetZ,
		&distance, &azimuth, &elevation,
		&eyeX, &eyeY, &eyeZ,
		&upX, &upY, &upZ,
		&minDistance, &maxDistance
	}) {
		row->slider->setEnabled(active);
		row->spin->setEnabled(active);
	}
}

void CameraControl::notifyCameraEdited()
{
	if (renderWidget == nullptr) {
		return;
	}

	auto* camera = renderWidget->modelOrbitCamera();
	if (camera == nullptr) {
		return;
	}

	syncFromCamera();
	renderWidget->notifyCameraChanged();
}

void CameraControl::syncFromCamera()
{
	if (renderWidget == nullptr) {
		setActive(false);
		return;
	}

	auto* camera = renderWidget->modelOrbitCamera();
	if (camera == nullptr) {
		setActive(false);
		return;
	}

	setActive(true);
	isLoadingUi = true;

	focusX.setter = [camera](float v) { camera->setFocus(glm::vec3(v, camera->focus().y, camera->focus().z)); };
	focusY.setter = [camera](float v) { camera->setFocus(glm::vec3(camera->focus().x, v, camera->focus().z)); };
	focusZ.setter = [camera](float v) { camera->setFocus(glm::vec3(camera->focus().x, camera->focus().y, v)); };
	focusX.getter = [camera]() { return camera->focus().x; };
	focusY.getter = [camera]() { return camera->focus().y; };
	focusZ.getter = [camera]() { return camera->focus().z; };

	offsetX.setter = [camera](float v) { camera->setOrbitOffset(glm::vec3(v, camera->orbitOffset().y, camera->orbitOffset().z)); };
	offsetY.setter = [camera](float v) { camera->setOrbitOffset(glm::vec3(camera->orbitOffset().x, v, camera->orbitOffset().z)); };
	offsetZ.setter = [camera](float v) { camera->setOrbitOffset(glm::vec3(camera->orbitOffset().x, camera->orbitOffset().y, v)); };
	offsetX.getter = [camera]() { return camera->orbitOffset().x; };
	offsetY.getter = [camera]() { return camera->orbitOffset().y; };
	offsetZ.getter = [camera]() { return camera->orbitOffset().z; };

	distance.setter = [camera](float v) {
		camera->setOrbitSpherical(camera->azimuthDegrees(), camera->elevationDegrees(), v);
	};
	azimuth.setter = [camera](float v) {
		camera->setOrbitSpherical(v, camera->elevationDegrees(), camera->distance());
	};
	elevation.setter = [camera](float v) {
		camera->setOrbitSpherical(camera->azimuthDegrees(), v, camera->distance());
	};
	distance.getter = [camera]() { return camera->distance(); };
	azimuth.getter = [camera]() { return camera->azimuthDegrees(); };
	elevation.getter = [camera]() { return camera->elevationDegrees(); };

	eyeX.setter = [camera](float v) { camera->setEyePosition(glm::vec3(v, camera->eyePosition().y, camera->eyePosition().z)); };
	eyeY.setter = [camera](float v) { camera->setEyePosition(glm::vec3(camera->eyePosition().x, v, camera->eyePosition().z)); };
	eyeZ.setter = [camera](float v) { camera->setEyePosition(glm::vec3(camera->eyePosition().x, camera->eyePosition().y, v)); };
	eyeX.getter = [camera]() { return camera->eyePosition().x; };
	eyeY.getter = [camera]() { return camera->eyePosition().y; };
	eyeZ.getter = [camera]() { return camera->eyePosition().z; };

	upX.setter = [camera](float v) { camera->setUpVector(glm::vec3(v, camera->upVector().y, camera->upVector().z)); };
	upY.setter = [camera](float v) { camera->setUpVector(glm::vec3(camera->upVector().x, v, camera->upVector().z)); };
	upZ.setter = [camera](float v) { camera->setUpVector(glm::vec3(camera->upVector().x, camera->upVector().y, v)); };
	upX.getter = [camera]() { return camera->upVector().x; };
	upY.getter = [camera]() { return camera->upVector().y; };
	upZ.getter = [camera]() { return camera->upVector().z; };

	minDistance.setter = [camera](float v) { camera->setMinDistance(v); };
	maxDistance.setter = [camera](float v) { camera->setMaxDistance(v); };
	minDistance.getter = [camera]() { return camera->minDistance(); };
	maxDistance.getter = [camera]() { return camera->maxDistance(); };

	auto setAxis = [](AxisRow& row, float value) {
		row.spin->setValue(value);
		row.slider->setValue(static_cast<int>(value * row.sliderScale));
	};

	const glm::vec3 focus = camera->focus();
	setAxis(focusX, focus.x);
	setAxis(focusY, focus.y);
	setAxis(focusZ, focus.z);

	const glm::vec3 offset = camera->orbitOffset();
	setAxis(offsetX, offset.x);
	setAxis(offsetY, offset.y);
	setAxis(offsetZ, offset.z);

	setAxis(distance, camera->distance());
	setAxis(azimuth, camera->azimuthDegrees());
	setAxis(elevation, camera->elevationDegrees());

	const glm::vec3 eye = camera->eyePosition();
	setAxis(eyeX, eye.x);
	setAxis(eyeY, eye.y);
	setAxis(eyeZ, eye.z);

	const glm::vec3 up = camera->upVector();
	setAxis(upX, up.x);
	setAxis(upY, up.y);
	setAxis(upZ, up.z);

	setAxis(minDistance, camera->minDistance());
	setAxis(maxDistance, camera->maxDistance());

	checkBoxFollowModel->setChecked(renderWidget->followsModelFocus());

	isLoadingUi = false;
}
