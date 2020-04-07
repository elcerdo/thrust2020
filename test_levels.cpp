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

    const auto data = levels::load(":/levels/levels.json");

    require(data.default_level >= -1, "invalid default_level");
    require(data.default_level < static_cast<int>(data.levels.size()), "invalid default_level");

    cout << data.levels.size() << " levels" << endl;
    int index = 0;
    for (const auto& level : data.levels)
    {
        const auto map_exists = QFileInfo(QString::fromStdString(level.map_filename)).exists();

        cout << (index == data.default_level ? "->" : "  ") << " ";
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
        index++;
    }

    return 0;
}


