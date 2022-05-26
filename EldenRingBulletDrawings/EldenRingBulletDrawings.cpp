#include "EldenRingBulletDrawings.h"

EldenRingBulletDrawings::EldenRingBulletDrawings(QWidget *parent)
    : QWidget(parent)
{
    QLocale::setDefault(QLocale(QLocale::English));
    setWindowTitle("Elden Ring Bullet Drawer @By Mephisto");

    resize(500, 700);

    QFile styleFile(":/EldenRingBulletDrawings/style");
    styleFile.open(QIODevice::ReadOnly);
    QString style(styleFile.readAll());
    setStyleSheet(style);

    setAutoFillBackground(true);
    QPalette pal;
    pal.setBrush(backgroundRole(), QBrush(QPixmap(":/EldenRingBulletDrawings/space")));
    setPalette(pal);

    controlMenu = new ControlMenu();
    net = new DrawingNet();

    QScrollArea* netSA = new QScrollArea();
    netSA->setWidget(net);

    draw = new QPushButton("Draw");
    draw->setFixedHeight(40);
    buttonSetter = new ButtonSetter();
    buttonSetter->setPlaceholderText("Hotkey");
    buttonSetter->setFixedHeight(40);
    QFont font = buttonSetter->font();
    font.setPointSizeF(18);
    buttonSetter->setFont(font);
    buttonSetter->setAlignment(Qt::AlignCenter);

    QHBoxLayout* drawLayout = new QHBoxLayout();
    drawLayout->addWidget(buttonSetter, 2);
    drawLayout->addWidget(draw, 3);
    drawLayout->setContentsMargins(10, 10, 10, 10);
    drawLayout->setSpacing(20);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(controlMenu);
    layout->addWidget(netSA);
    layout->addLayout(drawLayout);
    layout->setMargin(0);
    layout->setSpacing(0);

    setLayout(layout);
}
