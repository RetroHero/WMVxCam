#pragma once

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <functional>

class RenderWidget;

class SecondaryCameraControl : public QWidget
{
public:
	SecondaryCameraControl(QWidget* parent = nullptr);
	~SecondaryCameraControl();

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

	QPushButton* pushButtonResetSecondary;
	QPushButton* pushButtonMirrorOpposite;

	AxisRow orbitAngle;
	AxisRow elevation;
	AxisRow distance;

	AxisRow offsetX;
	AxisRow offsetY;
	AxisRow offsetZ;

	AxisRow eyeX;
	AxisRow eyeY;
	AxisRow eyeZ;
};
