#pragma once
#include <QtWidgets>
#include <stdexcept>

#include "LineEdit.h"

class ControlMenu : public QWidget
{
	Q_OBJECT

public:
	ControlMenu(QWidget* pwgt = Q_NULLPTR);
	int getBulletId();
	float getSpace();

private:
	LineEdit* bulletId;
	LineEdit* space;
	LineEdit* xSize;
	LineEdit* ySize;
	LineEdit* zSize;
	QComboBox* currentLayer;
	QPushButton* set;

	void fillZSize();

protected:
	void paintEvent(QPaintEvent* e);

private slots:
	void setNetSize();

signals:
	void netSizeChanged(int xSize, int ySize, int zSize);
	void currentLayerChanged(int layer);
};

