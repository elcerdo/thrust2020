#include "load_levels.h"

#include <QJsonDocument>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDebug>

QJsonObject load_json_object(const std::string& json_filename)
{
    QFile inFile(QString::fromStdString(json_filename));
    inFile.open(QIODevice::ReadOnly|QIODevice::Text);
    const QByteArray data = inFile.readAll();
    inFile.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (doc.isNull())
        qDebug() << error.errorString();
    assert(!doc.isNull());

    return doc.object();
}

float float_from_json(const QJsonValue& obj, const QString& name, const float def = 0)
{
    return static_cast<float>(obj.toObject()[name].toDouble(def));
}

b2Vec2 vec2_from_json(const QJsonValue& obj, const QString& xx_name, const QString& yy_name, const b2Vec2 def = { 0, 0 } )
{
    return { float_from_json(obj, xx_name, def.x), float_from_json(obj, yy_name, def.y) };
}

levels::MainData levels::load(const std::string& json_filename)
{

    const auto root_obj = load_json_object(json_filename);

    MainData::LevelDatas levels;
    for (const auto& level_json : root_obj["levels"].toArray())
    {
        assert(level_json.isObject());
        const auto& level_obj = level_json.toObject();

        LevelData level {
            level_obj["name"].toString().toStdString(),
            level_obj["map"].toString().toStdString(),
            {},
            {},
            vec2_from_json(level_obj, "wcx", "wcy", b2Vec2 { 0, -120 }),
            float_from_json(level_obj, "wsh", 500),
        };

        for (const auto& door_json : level_obj["doors"].toArray())
        {
            assert(door_json.isObject());
            const auto& door_obj = door_json.toObject();
            const b2Vec2 center = vec2_from_json(door_obj, "cx", "cy");
            const b2Vec2 size = vec2_from_json(door_obj, "ww", "hh", b2Vec2 { 1, 10 });
            const b2Vec2 delta = vec2_from_json(door_obj, "dx", "dy", b2Vec2 { 0, -20 });
            level.doors.emplace_back(LevelData::DoorData { center, size, delta });
        }

        for (const auto& path_json : level_obj["paths"].toArray())
        {
            assert(path_json.isObject());
            const auto& path_obj = path_json.toObject();
            const b2Vec2 size = vec2_from_json(path_obj, "ww", "hh", b2Vec2 { 1, 10 });

            std::vector<b2Vec2> positions;
            for (const auto& pos_json : path_obj["positions"].toArray())
                positions.emplace_back(vec2_from_json(pos_json, "x", "y"));

            level.paths.emplace_back(LevelData::PathData { positions, size });
        }

        levels.emplace_back(level);
    }

    return { levels, root_obj["default_level"].toInt(-1) };
}

