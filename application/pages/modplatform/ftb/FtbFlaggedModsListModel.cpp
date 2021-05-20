#include "FtbFlaggedModsListModel.h"

FlaggedModsListModel::FlaggedModsListModel(QWidget *parent, QVector<ModpacksCH::FlaggedMod> flaggedMods)
    : QAbstractListModel(parent), m_flaggedMods(flaggedMods)
{
    // fill in defaults
    for (const auto& mod : flaggedMods) {
        m_modState[mod.fileName] = false;
    }
}

int FlaggedModsListModel::rowCount(const QModelIndex &parent) const {
    return m_flaggedMods.size();
}

int FlaggedModsListModel::columnCount(const QModelIndex &parent) const {
    // enable/disable, name, flag reason, description
    return 4;
}

QVariant FlaggedModsListModel::data(const QModelIndex &index, int role) const {
    auto row = index.row();
    auto mod = m_flaggedMods.at(row);

    if (index.column() == EnabledColumn && role == Qt::CheckStateRole) {
        return m_modState[mod.fileName] ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::DisplayRole) {
        if (index.column() == FlaggedForColumn) {
            return mod.flagReason;
        }
        if (index.column() == NameColumn) {
            return mod.modName;
        }
        if (index.column() == DescriptionColumn) {
            return mod.modDescription;
        }
    }

    return QVariant();
}

bool FlaggedModsListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::CheckStateRole) {
        auto row = index.row();
        auto mod = m_flaggedMods.at(row);

        // toggle the state
        m_modState[mod.fileName] = !m_modState[mod.fileName];

        emit dataChanged(FlaggedModsListModel::index(row, 0),
                         FlaggedModsListModel::index(row, columnCount(QModelIndex()) - 1));
        return true;
    }

    return false;
}

QVariant FlaggedModsListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case EnabledColumn:
                return QString();
            case FlaggedForColumn:
                return QString("Flag Reason");
            case NameColumn:
                return QString("Name");
            case DescriptionColumn:
                return QString("Description");
        }
    }

    return QVariant();
}

void FlaggedModsListModel::enableDisableMods() {
    for (const auto& mod : m_flaggedMods) {
        if (!m_modState[mod.fileName]) {
            // Disable the mod by appending .disabled to filename
            QFile file(mod.fileName);
            if (!file.rename(mod.fileName + ".disabled")) {
                qWarning() << "Failed to disable" << mod.fileName;
            }
        }
    }
}

Qt::ItemFlags FlaggedModsListModel::flags(const QModelIndex &index) const {
    auto flags = QAbstractListModel::flags(index);
    if (index.isValid()) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}
