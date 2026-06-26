#pragma once

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <functional>

class RenderWidget;

class CameraControl : public QWidget
{
	Q_OBJECT

public:
	CameraControl(QWidget* parent = nullptr);
	~CameraControl();

	void setRenderWidget(RenderWidget* widget);

private:
	void buildUi();
	void bindControls();
	void setActive(bool active);
	void syncFromCamera();
	void notifyCameraEdited();

	struct AxisRow {
		QSlider* slider = nullptr;
		QDoubleSpinBox* spin = nullptr;
		float sliderScale = 100.f;
		std::function<void(float)> setter;
		std::function<float()> getter;
	};

	AxisRow addAxisRow(QFormLayout* layout, const QString& label, float min, float max, float step, float sliderScale);

	RenderWidget* renderWidget;
	bool isLoadingUi;
	bool isOrbitCamera;

	QCheckBox* checkBoxFollowModel;
	QPushButton* pushButtonReset;

	AxisRow focusX;
	AxisRow focusY;
	AxisRow focusZ;

	AxisRow offsetX;
	AxisRow offsetY;
	AxisRow offsetZ;

	AxisRow distance;
	AxisRow azimuth;
	AxisRow elevation;

	AxisRow eyeX;
	AxisRow eyeY;
	AxisRow eyeZ;

	AxisRow upX;
	AxisRow upY;
	AxisRow upZ;

	AxisRow minDistance;
	AxisRow maxDistance;
};
