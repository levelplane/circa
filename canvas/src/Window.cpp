
#include <QtGui>

#include "Window.h"

Window::Window()
{
    GLWidget *openGL = new GLWidget(&helper, this);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(openGL, 0, 0);
    setLayout(layout);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), openGL, SLOT(animate()));
    timer->start(16);

    setWindowTitle(tr("circaCanvas"));
}