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

#include "BaseEntity.h"
#include "BaseVersionList.h"
#include <QJsonObject>
#include <memory>

namespace Meta
{
using VersionPtr = std::shared_ptr<class Version>;
using VersionListPtr = std::shared_ptr<class VersionList>;

class MULTIMC_LOGIC_EXPORT VersionList : public BaseVersionList, public BaseEntity
{
    Q_OBJECT
    Q_PROPERTY(QString uid READ uid CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
    explicit VersionList(const QString &uid, QObject *parent = nullptr);

    enum Roles
    {
        UidRole = Qt::UserRole + 100,
        TimeRole,
        RequiresRole,
        VersionPtrRole
    };

    shared_qobject_ptr<Task> getLoadTask() override;
    bool isLoaded() override;
    const BaseVersionPtr at(int i) const override;
    int count() const override;
    void sortVersions() override;

    BaseVersionPtr getRecommended() const override;

    QVariant data(const QModelIndex &index, int role) const override;
    RoleList providesRoles() const override;
    QHash<int, QByteArray> roleNames() const override;

    QString localFilename() const override;

    QString uid() const
    {
        return m_uid;
    }
    QString name() const
    {
        return m_name;
    }
    QString humanReadable() const;

    VersionPtr getVersion(const QString &version);

    QVector<VersionPtr> versions() const
    {
        return m_versions;
    }

public: // for usage only by parsers
    void setName(const QString &name);
    void setVersions(const QVector<VersionPtr> &versions);
    void merge(const VersionListPtr &other);
    void mergeFromIndex(const VersionListPtr &other);
    void parse(const QJsonObject &obj) override;

signals:
    void nameChanged(const QString &name);

protected slots:
    void updateListData(QList<BaseVersionPtr>) override
    {
    }

private:
    QVector<VersionPtr> m_versions;
    QHash<QString, VersionPtr> m_lookup;
    QString m_uid;
    QString m_name;

    VersionPtr m_recommended;

    void setupAddedVersion(const int row, const VersionPtr &version);
};
}
Q_DECLARE_METATYPE(Meta::VersionListPtr)
