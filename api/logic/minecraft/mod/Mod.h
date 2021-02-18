/* Copyright 2013-2021 MultiMC Contributors
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
#include <QList>
#include <memory>

#include "multimc_logic_export.h"

#include "ModDetails.h"



class MULTIMC_LOGIC_EXPORT Mod
{
public:
    enum ModType
    {
        MOD_UNKNOWN,    //!< Indicates an unspecified mod type.
        MOD_ZIPFILE,    //!< The mod is a zip file containing the mod's class files.
        MOD_SINGLEFILE, //!< The mod is a single file (not a zip file).
        MOD_FOLDER,     //!< The mod is in a folder on the filesystem.
        MOD_LITEMOD, //!< The mod is a litemod
    };

    Mod() = default;
    Mod(const QFileInfo &file);

    QFileInfo filename() const
    {
        return m_file;
    }
    QString mmc_id() const
    {
        return m_mmc_id;
    }
    ModType type() const
    {
        return m_type;
    }
    bool valid()
    {
        return m_type != MOD_UNKNOWN;
    }

    QDateTime dateTimeChanged() const
    {
        return m_changedDateTime;
    }

    bool enabled() const
    {
        return m_enabled;
    }

    const ModDetails &details() const;

    QString name() const;
    QString version() const;
    QString homeurl() const;
    QString description() const;
    QStringList authors() const;

    bool enable(bool value);

    // delete all the files of this mod
    bool destroy();

    // change the mod's filesystem path (used by mod lists for *MAGIC* purposes)
    void repath(const QFileInfo &file);

    bool shouldResolve() {
        return !m_resolving && !m_resolved;
    }
    bool isResolving() {
        return m_resolving;
    }
    int resolutionTicket()
    {
        return m_resolutionTicket;
    }
    void setResolving(bool resolving, int resolutionTicket) {
        m_resolving = resolving;
        m_resolutionTicket = resolutionTicket;
    }
    void finishResolvingWithDetails(std::shared_ptr<ModDetails> details){
        m_resolving = false;
        m_resolved = true;
        m_localDetails = details;
    }

protected:
    QFileInfo m_file;
    QDateTime m_changedDateTime;
    QString m_mmc_id;
    QString m_name;
    bool m_enabled = true;
    bool m_resolving = false;
    bool m_resolved = false;
    int m_resolutionTicket = 0;
    ModType m_type = MOD_UNKNOWN;
    std::shared_ptr<ModDetails> m_localDetails;
};
