#include "GameWindow.h"

#include <QApplication>

#include <iostream>

int main(int argc, char* argv[])
{
    using std::cout;
    cout << std::boolalpha;

    QApplication app(argc, argv);

    GameWindow main;
    main.show();

    return app.exec();
}

