#include "GameWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
		QApplication app(argc, argv);

		GameWindow main;
		main.show();

		return app.exec();
}

