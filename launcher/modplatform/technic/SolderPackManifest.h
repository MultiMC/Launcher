/*
 * Copyright 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>

namespace TechnicSolder {

struct Pack {
    QString recommended;
    QString latest;
    QVector<QString> builds;
};

void loadPack(Pack& v, QJsonObject& obj);

struct PackBuildMod {
    QString name;
    QString version;
    QString md5;
    QString url;
};

struct PackBuild {
    QString minecraft;
    QVector<PackBuildMod> mods;
};

void loadPackBuild(PackBuild& v, QJsonObject& obj);

}
