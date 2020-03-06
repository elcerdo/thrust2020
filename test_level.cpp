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

QJsonObject loadJsonObject(const QString& filename)
{
    QFile inFile(filename);
    inFile.open(QIODevice::ReadOnly|QIODevice::Text);
    QByteArray data = inFile.readAll();
    inFile.close();

    QJsonParseError errorPtr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
    if (doc.isNull()) {
        qDebug() << "Parse failed";
    }
    QJsonObject rootObj = doc.object();

    return rootObj;
}

b2Vec2 vec2FromJson(const QJsonValue& obj, const QString& xx_name, const QString& yy_name, const b2Vec2 def = { 0, 0 } )
{
    return  { static_cast<float>(obj.toObject()[xx_name].toDouble(def.x)), static_cast<float>(obj.toObject()[yy_name].toDouble(def.y)) };
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QJsonObject obj = loadJsonObject(":level1.json");

    qDebug() << obj.isEmpty();
    qDebug() << obj;

    for (const auto& door : obj["doors"].toArray())
    {
        const b2Vec2 center = vec2FromJson(door, "cx", "cy");
        const b2Vec2 size = vec2FromJson(door, "ww", "hh", b2Vec2 { 1, 10 });
        const b2Vec2 delta = vec2FromJson(door, "dx", "dy", b2Vec2 { 0, -20 });
        qDebug() << center.x << center.y << size.x << size.y << delta.x << delta.y;
    }


    return 0;
}


