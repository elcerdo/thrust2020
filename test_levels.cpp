#include "load_levels.h"

#include <QCoreApplication>
#include <QFileInfo>

#include <iostream>
#include <iomanip>

template <typename BB>
void
require(const BB cond, const std::string& message)
{
    if (!static_cast<bool>(cond))
        throw std::runtime_error(message);
}

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;
    using std::get;

    cout << std::boolalpha;

    QCoreApplication app(argc, argv);

    const auto levels = levels::load(":/levels/levels.json");

    cout << levels.size() << " levels" << endl;
    for (const auto& level : levels)
    {
        const auto map_exists = QFileInfo(QString::fromStdString(level.map_filename)).exists();

        cout << std::quoted(level.name) << " ";
        cout << std::quoted(level.map_filename) << " " << map_exists << " ";
        cout << level.doors.size() << "doors ";
        cout << level.paths.size() << "paths" << endl;

        require(map_exists, "map does not exists");

        /*
        size_t kk = 0;
        for (const auto& path : level.paths)
        {
            const auto& positions = get<0>(path);
            const auto& size = get<1>(path);
            cout << "** path " << kk++ << " " << positions.size() << " " << size.x << " " << size.y << endl;
        }
        */
    }

    return 0;
}


