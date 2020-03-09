#include "load_levels.h"

#include <QCoreApplication>

#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;

    QCoreApplication app(argc, argv);

    const auto levels = levels::load(":levels.json");

    cout << levels.size() << " levels" << endl;
    for (const auto& level : levels)
        cout << std::quoted(level.name) << " " << std::quoted(level.map_filename) << " " << level.doors.size() << "doors" << endl;

    return 0;
}


