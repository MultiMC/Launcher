/* Copyright 2015-2021 MultiMC Contributors
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
#include <QFileInfo>
#include <QDateTime>
#include <nonstd/optional>

#include "multimc_logic_export.h"

struct MULTIMC_LOGIC_EXPORT GameType {
    GameType() = default;
    GameType (nonstd::optional<int> original);

    QString toTranslatedString() const;
    QString toLogString() const;

    enum
    {
        Unknown = -1,
        Survival = 0,
        Creative,
        Adventure,
        Spectator
    } type = Unknown;
    nonstd::optional<int> original;
};

class MULTIMC_LOGIC_EXPORT World
{
public:
    World(const QFileInfo &file);
    QString folderName() const
    {
        return m_folderName;
    }
    QString name() const
    {
        return m_actualName;
    }
    QString iconFile() const
    {
        return m_iconFile;
    }
    QDateTime lastPlayed() const
    {
        return m_lastPlayed;
    }
    GameType gameType() const
    {
        return m_gameType;
    }
    int64_t seed() const
    {
        return m_randomSeed;
    }
    bool isValid() const
    {
        return is_valid;
    }
    bool isOnFS() const
    {
        return m_containerFile.isDir();
    }
    QFileInfo container() const
    {
        return m_containerFile;
    }
    // delete all the files of this world
    bool destroy();
    // replace this world with a copy of the other
    bool replace(World &with);
    // change the world's filesystem path (used by world lists for *MAGIC* purposes)
    void repath(const QFileInfo &file);
    // remove the icon file, if any
    bool resetIcon();

    bool rename(const QString &to);
    bool install(const QString &to, const QString &name= QString());

    // WEAK compare operator - used for replacing worlds
    bool operator==(const World &other) const;

private:
    void readFromZip(const QFileInfo &file);
    void readFromFS(const QFileInfo &file);
    void loadFromLevelDat(QByteArray data);

protected:

    QFileInfo m_containerFile;
    QString m_containerOffsetPath;
    QString m_folderName;
    QString m_actualName;
    QString m_iconFile;
    QDateTime levelDatTime;
    QDateTime m_lastPlayed;
    int64_t m_randomSeed = 0;
    GameType m_gameType;
    bool is_valid = false;
};
