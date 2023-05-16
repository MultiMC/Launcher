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

#include "SolderPackManifest.h"

#include "Json.h"

namespace TechnicSolder {

void loadPack(Pack& v, QJsonObject& obj)
{
    v.recommended = Json::requireString(obj, "recommended");
    v.latest = Json::requireString(obj, "latest");

    auto builds = Json::requireArray(obj, "builds");
    for (const auto buildRaw : builds) {
        auto build = Json::requireValueString(buildRaw);
        v.builds.append(build);
    }
}

static void loadPackBuildMod(PackBuildMod& b, QJsonObject& obj)
{
    b.name = Json::requireString(obj, "name");
    b.version = Json::requireString(obj, "version");
    b.md5 = Json::requireString(obj, "md5");
    b.url = Json::requireString(obj, "url");
}

void loadPackBuild(PackBuild& v, QJsonObject& obj)
{
    v.minecraft = Json::requireString(obj, "minecraft");

    auto mods = Json::requireArray(obj, "mods");
    for (const auto modRaw : mods) {
        auto modObj = Json::requireValueObject(modRaw);
        PackBuildMod mod;
        loadPackBuildMod(mod, modObj);
        v.mods.append(mod);
    }
}

}
