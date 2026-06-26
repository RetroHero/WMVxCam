#include "stdafx.h"
#include "SecondaryCameraControl.h"
#include "RenderWidget.h"
#include "ModelOrbitCamera.h"
#include <glm/glm.hpp>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

SecondaryCameraControl::SecondaryCameraControl(QWidget* parent)
	: QWidget(parent),
	renderWidget(nullptr),
	isLoadingUi(false),
	isOrbitCamera(false)
{
	buildUi();
	bindControls();
	setActive(false);
}

SecondaryCameraControl::~SecondaryCameraControl()
{}

void SecondaryCameraControl::setRenderWidget(RenderWidget* widget)
{
	if (renderWidget != nullptr) {
		disconnect(renderWidget, nullptr, this, nullptr);
	}

	renderWidget = widget;

	if (renderWidget != nullptr) {
		connect(renderWidget, &RenderWidget::cameraChanged, this, &SecondaryCameraControl::syncFromCamera);
		syncFromCamera();
	}
	else {
		setActive(false);
	}
}

SecondaryCameraControl::AxisRow SecondaryCameraControl::addAxisRow(QFormLayout* layout, const QString& label, float min, float max, float step, float sliderScale)
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

void SecondaryCameraControl::buildUi()
{
	auto* outerLayout = new QVBoxLayout(this);
	outerLayout->setContentsMargins(0, 0, 0, 0);

	auto* scrollArea = new QScrollArea(this);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setWidgetResizable(true);

	auto* contents = new QWidget(scrollArea);
	auto* layout = new QVBoxLayout(contents);

	auto* header = new QLabel(QStringLiteral("Adjusts only the right viewport. The left (primary) camera is unchanged."), contents);
	header->setWordWrap(true);
	layout->addWidget(header);

	auto* optionsGroup = new QGroupBox(QStringLiteral("Second camera (right view)"), contents);
	auto* optionsLayout = new QVBoxLayout(optionsGroup);
	pushButtonResetSecondary = new QPushButton(QStringLiteral("Reset second camera"), optionsGroup);
	pushButtonMirrorOpposite = new QPushButton(QStringLiteral("Mirror opposite of primary"), optionsGroup);
	optionsLayout->addWidget(pushButtonResetSecondary);
	optionsLayout->addWidget(pushButtonMirrorOpposite);
	layout->addWidget(optionsGroup);

	auto* rotationGroup = new QGroupBox(QStringLiteral("Orbit around model"), contents);
	auto* rotationLayout = new QFormLayout(rotationGroup);
	orbitAngle = addAxisRow(rotationLayout, QStringLiteral("Azimuth (deg)"), 0.f, 360.f, 0.1f, 10.f);
	elevation = addAxisRow(rotationLayout, QStringLiteral("Elevation (deg)"), -90.f, 90.f, 0.1f, 10.f);
	distance = addAxisRow(rotationLayout, QStringLiteral("Distance"), 0.f, 200.f, 0.01f, 100.f);
	layout->addWidget(rotationGroup);

	auto* offsetGroup = new QGroupBox(QStringLiteral("Orbit offset"), contents);
	auto* offsetLayout = new QFormLayout(offsetGroup);
	offsetX = addAxisRow(offsetLayout, QStringLiteral("X"), -100.f, 100.f, 0.01f, 100.f);
	offsetY = addAxisRow(offsetLayout, QStringLiteral("Y"), -100.f, 100.f, 0.01f, 100.f);
	offsetZ = addAxisRow(offsetLayout, QStringLiteral("Z"), -100.f, 100.f, 0.01f, 100.f);
	layout->addWidget(offsetGroup);

	auto* eyeGroup = new QGroupBox(QStringLiteral("Eye position"), contents);
	auto* eyeLayout = new QFormLayout(eyeGroup);
	eyeX = addAxisRow(eyeLayout, QStringLiteral("X"), -100.f, 100.f, 0.01f, 100.f);
	eyeY = addAxisRow(eyeLayout, QStringLiteral("Y"), -100.f, 100.f, 0.01f, 100.f);
	eyeZ = addAxisRow(eyeLayout, QStringLiteral("Z"), -100.f, 100.f, 0.01f, 100.f);
	layout->addWidget(eyeGroup);

	layout->addStretch(1);
	scrollArea->setWidget(contents);
	outerLayout->addWidget(scrollArea);
}

void SecondaryCameraControl::bindControls()
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
		&orbitAngle, &elevation, &distance,
		&offsetX, &offsetY, &offsetZ,
		&eyeX, &eyeY, &eyeZ
	}) {
		bindAxis(*row);
	}

	connect(pushButtonResetSecondary, &QPushButton::clicked, this, [this]() {
		if (renderWidget == nullptr) {
			return;
		}
		auto* camera = renderWidget->modelOrbitCamera();
		if (camera == nullptr) {
			return;
		}
		camera->resetSecondaryView();
		renderWidget->notifyCameraChanged();
	});

	connect(pushButtonMirrorOpposite, &QPushButton::clicked, this, [this]() {
		if (renderWidget == nullptr) {
			return;
		}
		auto* camera = renderWidget->modelOrbitCamera();
		if (camera == nullptr) {
			return;
		}
		camera->mirrorSecondaryToOpposite();
		notifyCameraEdited();
	});
}

void SecondaryCameraControl::setActive(bool active)
{
	isOrbitCamera = active;
	pushButtonResetSecondary->setEnabled(active);
	pushButtonMirrorOpposite->setEnabled(active);

	for (auto* row : {
		&orbitAngle, &elevation, &distance,
		&offsetX, &offsetY, &offsetZ,
		&eyeX, &eyeY, &eyeZ
	}) {
		row->slider->setEnabled(active);
		row->spin->setEnabled(active);
	}
}

void SecondaryCameraControl::notifyCameraEdited()
{
	syncFromCamera();
	if (renderWidget != nullptr) {
		renderWidget->notifyCameraChanged();
	}
}

void SecondaryCameraControl::syncFromCamera()
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

	orbitAngle.setter = [camera](float v) {
		camera->setSecondaryOrbitSpherical(v, camera->secondaryElevationDegrees(), camera->secondaryDistance());
	};
	elevation.setter = [camera](float v) {
		camera->setSecondaryOrbitSpherical(camera->secondaryAzimuthDegrees(), v, camera->secondaryDistance());
	};
	distance.setter = [camera](float v) {
		camera->setSecondaryOrbitSpherical(camera->secondaryAzimuthDegrees(), camera->secondaryElevationDegrees(), v);
	};
	orbitAngle.getter = [camera]() { return camera->secondaryAzimuthDegrees(); };
	elevation.getter = [camera]() { return camera->secondaryElevationDegrees(); };
	distance.getter = [camera]() { return camera->secondaryDistance(); };

	offsetX.setter = [camera](float v) {
		camera->setSecondaryOrbitOffset(glm::vec3(v, camera->secondaryOrbitOffset().y, camera->secondaryOrbitOffset().z));
	};
	offsetY.setter = [camera](float v) {
		camera->setSecondaryOrbitOffset(glm::vec3(camera->secondaryOrbitOffset().x, v, camera->secondaryOrbitOffset().z));
	};
	offsetZ.setter = [camera](float v) {
		camera->setSecondaryOrbitOffset(glm::vec3(camera->secondaryOrbitOffset().x, camera->secondaryOrbitOffset().y, v));
	};
	offsetX.getter = [camera]() { return camera->secondaryOrbitOffset().x; };
	offsetY.getter = [camera]() { return camera->secondaryOrbitOffset().y; };
	offsetZ.getter = [camera]() { return camera->secondaryOrbitOffset().z; };

	eyeX.setter = [camera](float v) {
		camera->setSecondaryEyePosition(glm::vec3(v, camera->secondaryEyePosition().y, camera->secondaryEyePosition().z));
	};
	eyeY.setter = [camera](float v) {
		camera->setSecondaryEyePosition(glm::vec3(camera->secondaryEyePosition().x, v, camera->secondaryEyePosition().z));
	};
	eyeZ.setter = [camera](float v) {
		camera->setSecondaryEyePosition(glm::vec3(camera->secondaryEyePosition().x, camera->secondaryEyePosition().y, v));
	};
	eyeX.getter = [camera]() { return camera->secondaryEyePosition().x; };
	eyeY.getter = [camera]() { return camera->secondaryEyePosition().y; };
	eyeZ.getter = [camera]() { return camera->secondaryEyePosition().z; };

	auto setAxis = [](AxisRow& row, float value) {
		row.spin->setValue(value);
		row.slider->setValue(static_cast<int>(value * row.sliderScale));
	};

	setAxis(orbitAngle, camera->secondaryAzimuthDegrees());
	setAxis(elevation, camera->secondaryElevationDegrees());
	setAxis(distance, camera->secondaryDistance());

	const glm::vec3 offset = camera->secondaryOrbitOffset();
	setAxis(offsetX, offset.x);
	setAxis(offsetY, offset.y);
	setAxis(offsetZ, offset.z);

	const glm::vec3 eye = camera->secondaryEyePosition();
	setAxis(eyeX, eye.x);
	setAxis(eyeY, eye.y);
	setAxis(eyeZ, eye.z);

	isLoadingUi = false;
}
