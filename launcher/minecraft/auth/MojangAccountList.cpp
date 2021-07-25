#include "AccountList.h"
#include "Account.h"

#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>

#include <QDebug>

#include <FileSystem.h>

#define ACCOUNT_LIST_FORMAT_VERSION 2

AccountList::AccountList(QObject *parent) : QAbstractListModel(parent)
{
}

AccountPtr AccountList::findAccount(const QString &username) const
{
    for (int i = 0; i < count(); i++)
    {
        AccountPtr account = at(i);
        if (account->username() == username)
            return account;
    }
    return nullptr;
}

const AccountPtr AccountList::at(int i) const
{
    return AccountPtr(m_accounts.at(i));
}

void AccountList::addAccount(const AccountPtr account)
{
    int row = m_accounts.count();
    beginInsertRows(QModelIndex(), row, row);
    connect(account.get(), SIGNAL(changed()), SLOT(accountChanged()));
    m_accounts.append(account);
    endInsertRows();
    onListChanged();
}

void AccountList::removeAccount(const QString &username)
{
    int idx = 0;
    for (auto account : m_accounts)
    {
        if (account->username() == username)
        {
            beginRemoveRows(QModelIndex(), idx, idx);
            m_accounts.removeOne(account);
            endRemoveRows();
            return;
        }
        idx++;
    }
    onListChanged();
}

void AccountList::removeAccount(QModelIndex index)
{
    int row = index.row();
    if(index.isValid() && row >= 0 && row < m_accounts.size())
    {
        auto & account = m_accounts[row];
        if(account == m_activeAccount)
        {
            m_activeAccount = nullptr;
            onActiveChanged();
        }
        beginRemoveRows(QModelIndex(), row, row);
        m_accounts.removeAt(index.row());
        endRemoveRows();
        onListChanged();
    }
}

AccountPtr AccountList::activeAccount() const
{
    return m_activeAccount;
}

void AccountList::setActiveAccount(const QString &username)
{
    if (username.isEmpty() && m_activeAccount)
    {
        int idx = 0;
        auto prevActiveAcc = m_activeAccount;
        m_activeAccount = nullptr;
        for (AccountPtr account : m_accounts)
        {
            if (account == prevActiveAcc)
            {
                emit dataChanged(index(idx), index(idx));
            }
            idx ++;
        }
        onActiveChanged();
    }
    else
    {
        auto currentActiveAccount = m_activeAccount;
        int currentActiveAccountIdx = -1;
        auto newActiveAccount = m_activeAccount;
        int newActiveAccountIdx = -1;
        int idx = 0;
        for (AccountPtr account : m_accounts)
        {
            if (account->username() == username)
            {
                newActiveAccount = account;
                newActiveAccountIdx = idx;
            }
            if(currentActiveAccount == account)
            {
                currentActiveAccountIdx = idx;
            }
            idx++;
        }
        if(currentActiveAccount != newActiveAccount)
        {
            emit dataChanged(index(currentActiveAccountIdx), index(currentActiveAccountIdx));
            emit dataChanged(index(newActiveAccountIdx), index(newActiveAccountIdx));
            m_activeAccount = newActiveAccount;
            onActiveChanged();
        }
    }
}

void AccountList::accountChanged()
{
    // the list changed. there is no doubt.
    onListChanged();
}

void AccountList::onListChanged()
{
    if (m_autosave)
        // TODO: Alert the user if this fails.
        saveList();

    emit listChanged();
}

void AccountList::onActiveChanged()
{
    if (m_autosave)
        saveList();

    emit activeAccountChanged();
}

int AccountList::count() const
{
    return m_accounts.count();
}

QVariant AccountList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() > count())
        return QVariant();

    AccountPtr account = at(index.row());

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case NameColumn:
            return account->username();

        case TypeColumn:
            return account->provider()->displayName();

        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        return account->username();

    case PointerRole:
        return qVariantFromValue(account);

    case Qt::CheckStateRole:
        switch (index.column())
        {
        case ActiveColumn:
            return account == m_activeAccount ? Qt::Checked : Qt::Unchecked;
        }

    default:
        return QVariant();
    }
}

QVariant AccountList::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        switch (section)
        {
        case ActiveColumn:
            return tr("Active?");

        case NameColumn:
            return tr("Name");

        case TypeColumn:
            return tr("Account type");

        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        switch (section)
        {
        case NameColumn:
            return tr("The name of the version.");

        default:
            return QVariant();
        }

    default:
        return QVariant();
    }
}

int AccountList::rowCount(const QModelIndex &) const
{
    // Return count
    return count();
}

int AccountList::columnCount(const QModelIndex &) const
{
    return 3;
}

Qt::ItemFlags AccountList::flags(const QModelIndex &index) const
{
    if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool AccountList::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
    {
        return false;
    }

    if(role == Qt::CheckStateRole)
    {
        if(value == Qt::Checked)
        {
            AccountPtr account = this->at(index.row());
            this->setActiveAccount(account->username());
        }
    }

    emit dataChanged(index, index);
    return true;
}

void AccountList::updateListData(QList<AccountPtr> versions)
{
    beginResetModel();
    m_accounts = versions;
    endResetModel();
}

bool AccountList::loadList(const QString &filePath)
{
    QString path = filePath;
    if (path.isEmpty())
        path = m_listFilePath;
    if (path.isEmpty())
    {
        qCritical() << "Can't load Mojang account list. No file path given and no default set.";
        return false;
    }

    QFile file(path);

    // Try to open the file and fail if we can't.
    // TODO: We should probably report this error to the user.
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << QString("Failed to read the account list file (%1).").arg(path).toUtf8();
        return false;
    }

    // Read the file and close it.
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    // Fail if the JSON is invalid.
    if (parseError.error != QJsonParseError::NoError)
    {
        qCritical() << QString("Failed to parse account list file: %1 at offset %2")
                            .arg(parseError.errorString(), QString::number(parseError.offset))
                            .toUtf8();
        return false;
    }

    // Make sure the root is an object.
    if (!jsonDoc.isObject())
    {
        qCritical() << "Invalid account list JSON: Root should be an array.";
        return false;
    }

    QJsonObject root = jsonDoc.object();

    // Make sure the format version matches.
    if (root.value("formatVersion").toVariant().toInt() != ACCOUNT_LIST_FORMAT_VERSION)
    {
        QString newName = "accounts-old.json";
        qWarning() << "Format version mismatch when loading account list. Existing one will be renamed to"
                    << newName;

        // Attempt to rename the old version.
        file.rename(newName);
        return false;
    }

    // Now, load the accounts array.
    beginResetModel();
    QJsonArray accounts = root.value("accounts").toArray();
    for (QJsonValue accountVal : accounts)
    {
        QJsonObject accountObj = accountVal.toObject();
        AccountPtr account = Account::loadFromJson(accountObj);
        if (account.get() != nullptr)
        {
            connect(account.get(), SIGNAL(changed()), SLOT(accountChanged()));
            m_accounts.append(account);
        }
        else
        {
            qWarning() << "Failed to load an account.";
        }
    }
    // Load the active account.
    m_activeAccount = findAccount(root.value("activeAccount").toString(""));
    endResetModel();
    return true;
}

bool AccountList::saveList(const QString &filePath)
{
    QString path(filePath);
    if (path.isEmpty())
        path = m_listFilePath;
    if (path.isEmpty())
    {
        qCritical() << "Can't save Mojang account list. No file path given and no default set.";
        return false;
    }

    // make sure the parent folder exists
    if(!FS::ensureFilePathExists(path))
        return false;

    // make sure the file wasn't overwritten with a folder before (fixes a bug)
    QFileInfo finfo(path);
    if(finfo.isDir())
    {
        QDir badDir(path);
        badDir.removeRecursively();
    }

    qDebug() << "Writing account list to" << path;

    qDebug() << "Building JSON data structure.";
    // Build the JSON document to write to the list file.
    QJsonObject root;

    root.insert("formatVersion", ACCOUNT_LIST_FORMAT_VERSION);

    // Build a list of accounts.
    qDebug() << "Building account array.";
    QJsonArray accounts;
    for (AccountPtr account : m_accounts)
    {
        QJsonObject accountObj = account->saveToJson();
        accounts.append(accountObj);
    }

    // Insert the account list into the root object.
    root.insert("accounts", accounts);

    if(m_activeAccount)
    {
        // Save the active account.
        root.insert("activeAccount", m_activeAccount->username());
    }

    // Create a JSON document object to convert our JSON to bytes.
    QJsonDocument doc(root);

    // Now that we're done building the JSON object, we can write it to the file.
    qDebug() << "Writing account list to file.";
    QFile file(path);

    // Try to open the file and fail if we can't.
    // TODO: We should probably report this error to the user.
    if (!file.open(QIODevice::WriteOnly))
    {
        qCritical() << QString("Failed to read the account list file (%1).").arg(path).toUtf8();
        return false;
    }

    // Write the JSON to the file.
    file.write(doc.toJson());
    file.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ReadUser|QFile::WriteUser);
    file.close();

    qDebug() << "Saved account list to" << path;

    return true;
}

void AccountList::setListFilePath(QString path, bool autosave)
{
    m_listFilePath = path;
    m_autosave = autosave;
}

bool AccountList::anyAccountIsValid()
{
    for(auto account:m_accounts)
    {
        if(account->accountStatus() != NotVerified)
            return true;
    }
    return false;
}
