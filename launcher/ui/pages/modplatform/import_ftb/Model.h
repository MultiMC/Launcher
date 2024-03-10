/*
 * Copyright 2022 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <RWStorage.h>

#include <QAbstractListModel>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

#include <functional>
#include <nonstd/optional>

namespace ImportFTB {

enum class ModLoaderType {
    Unresolved,
    None,
    Forge,
    NeoForge,
    Fabric,
    Quilt
};

struct ModLoader {
    QString id;
    ModLoaderType type = ModLoaderType::Unresolved;
    QString version;
};

struct Modpack {
    int id = 0;
    int versionId = 0;

    QString name;
    QString version;
    QString description;
    QStringList authors;

    QString mcVersion;
    ModLoader modLoader;
    bool hasInstMods = false;

    int minMemory = 1024;
    int recMemory = 2048;
    int memory = 2048;

    QString instanceDir;
    QString iconPath;
    QIcon icon;

    QString getLogoName() {
        return QString("ftb_%1").arg(id);
    }
};

class Model : public QAbstractListModel
{
    Q_OBJECT
private:
    QList<Modpack> modpacks;

public:
    Model(QObject *parent);
    virtual ~Model() = default;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void reload();
};

}

Q_DECLARE_METATYPE(ImportFTB::Modpack)
