#include <Box2D/Common/b2Math.h>

#include <iostream>

#include <QJsonDocument>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QCoreApplication>

using std::cout;
using std::endl;

QJsonArray load_json_array(const std::string& json_filename)
{
    QFile inFile(QString::fromStdString(json_filename));
    inFile.open(QIODevice::ReadOnly|QIODevice::Text);
    const QByteArray data = inFile.readAll();
    inFile.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (doc.isNull()) {
        qDebug() << "json parsing failed";
        qDebug() << error.errorString();
    }
    assert(!doc.isNull());

    return doc.array();
}

b2Vec2 vec2_from_json(const QJsonValue& obj, const QString& xx_name, const QString& yy_name, const b2Vec2 def = { 0, 0 } )
{
    return  { static_cast<float>(obj.toObject()[xx_name].toDouble(def.x)), static_cast<float>(obj.toObject()[yy_name].toDouble(def.y)) };
}

struct Level
{
    std::string name = "";
    std::string map_filename = "";

    using DoorData = std::tuple<b2Vec2, b2Vec2, b2Vec2>;
    std::vector<DoorData> doors;
};

using Levels = std::vector<Level>;

Levels load_levels(const std::string& json_filename)
{
    Levels levels;

    for (const auto& level_json : load_json_array(":levels.json"))
    {
        assert(level_json.isObject());
        const auto& level_obj = level_json.toObject();
        qDebug() << "====================" << level_obj["name"].toString();
        qDebug() << level_obj["map"].toString();
        for (const auto& door_json : level_obj["doors"].toArray())
        {
            assert(door_json.isObject());
            const auto& door_obj = door_json.toObject();
            const b2Vec2 center = vec2_from_json(door_obj, "cx", "cy");
            const b2Vec2 size = vec2_from_json(door_obj, "ww", "hh", b2Vec2 { 1, 10 });
            const b2Vec2 delta = vec2_from_json(door_obj, "dx", "dy", b2Vec2 { 0, -20 });
            qDebug() << center.x << center.y << size.x << size.y << delta.x << delta.y;
        }



    }

    return levels;
}

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;

    QCoreApplication app(argc, argv);

    const auto levels = load_levels(":levels.json");
    for (const auto& level : levels)
        cout << level.name << " " << level.map_filename << " " << level.doors.size() << endl;

    return 0;
}


