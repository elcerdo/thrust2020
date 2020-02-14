#pragma once

class QWidget;
class QWindow;

namespace QtImGui {

void initialize(QWidget *window);
void initialize(QWindow *window);
void newFrame();

}
