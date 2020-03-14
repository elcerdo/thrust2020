#include "load_levels.h"

#include <QCoreApplication>

#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;
    using std::get;

    QCoreApplication app(argc, argv);

    const auto levels = levels::load(":levels.json");

    cout << levels.size() << " levels" << endl;
    for (const auto& level : levels)
    {
        cout << std::quoted(level.name) << " ";
        cout << std::quoted(level.map_filename) << " ";
        cout << level.doors.size() << "doors ";
        cout << level.paths.size() << "paths" << endl;

        size_t kk = 0;
        for (const auto& path : level.paths)
        {
            const auto& positions = get<0>(path);
            const auto& size = get<1>(path);
            cout << "** path " << kk++ << " " << positions.size() << " " << size.x << " " << size.y << endl;
        }
    }

    return 0;
}


