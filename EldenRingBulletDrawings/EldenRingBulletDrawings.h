#pragma once
#include <QtWidgets>

#include "ControlMenu.h"
#include "DrawingNet.h"
#include "ButtonSetter.h"

class EldenRingBulletDrawings : public QWidget
{
    Q_OBJECT

public:
    EldenRingBulletDrawings(QWidget *parent = Q_NULLPTR);

private:
    ControlMenu* controlMenu;
    DrawingNet* net;
    QPushButton* draw;
    ButtonSetter* buttonSetter;
};
