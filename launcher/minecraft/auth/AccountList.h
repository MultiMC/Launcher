#pragma once

#include "Account.h"

#include <QObject>
#include <QVariant>
#include <QAbstractListModel>
#include <QSharedPointer>

class MojangAccountList : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ModelRoles
    {
        PointerRole = 0x34B1CB48
    };

    enum VListColumns
    {
        // TODO: Add icon column.

        // First column - Active?
        ActiveColumn = 0,

        // Second column - Name
        NameColumn,

        // Third column - account type
        TypeColumn,
    };

    explicit AccountList(QObject *parent = 0);

    //! Gets the account at the given index.
    virtual const AccountPtr at(int i) const;

    //! Returns the number of accounts in the list.
    virtual int count() const;

    //////// List Model Functions ////////
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    /*!
     * Adds a the given Mojang account to the account list.
     */
    virtual void addAccount(const AccountPtr account);

    /*!
     * Removes the mojang account with the given username from the account list.
     */
    virtual void removeAccount(const QString &username);

    /*!
     * Removes the account at the given QModelIndex.
     */
    virtual void removeAccount(QModelIndex index);

    /*!
     * \brief Finds an account by its username.
     * \param The username of the account to find.
     * \return A const pointer to the account with the given username. NULL if
     * one doesn't exist.
     */
    virtual AccountPtr findAccount(const QString &username) const;

    /*!
     * Sets the default path to save the list file to.
     * If autosave is true, this list will automatically save to the given path whenever it changes.
     * THIS FUNCTION DOES NOT LOAD THE LIST. If you set autosave, be sure to call loadList() immediately
     * after calling this function to ensure an autosaved change doesn't overwrite the list you intended
     * to load.
     */
    virtual void setListFilePath(QString path, bool autosave = false);

    /*!
     * \brief Loads the account list from the given file path.
     * If the given file is an empty string (default), will load from the default account list file.
     * \return True if successful, otherwise false.
     */
    virtual bool loadList(const QString &file = "");

    /*!
     * \brief Saves the account list to the given file.
     * If the given file is an empty string (default), will save from the default account list file.
     * \return True if successful, otherwise false.
     */
    virtual bool saveList(const QString &file = "");

    /*!
     * \brief Gets a pointer to the account that the user has selected as their "active" account.
     * Which account is active can be overridden on a per-instance basis, but this will return the one that
     * is set as active globally.
     * \return The currently active Account. If there isn't an active account, returns a null pointer.
     */
    virtual AccountPtr activeAccount() const;

    /*!
     * Sets the given account as the current active account.
     * If the username given is an empty string, sets the active account to nothing.
     */
    virtual void setActiveAccount(const QString &username);

    /*!
     * Returns true if any of the account is at least Validated
     */
    bool anyAccountIsValid();

signals:
    /*!
     * Signal emitted to indicate that the account list has changed.
     * This will also fire if the value of an element in the list changes (will be implemented
     * later).
     */
    void listChanged();

    /*!
     * Signal emitted to indicate that the active account has changed.
     */
    void activeAccountChanged();

public
slots:
    /**
     * This is called when one of the accounts changes and the list needs to be updated
     */
    void accountChanged();

protected:
    /*!
     * Called whenever the list changes.
     * This emits the listChanged() signal and autosaves the list (if autosave is enabled).
     */
    void onListChanged();

    /*!
     * Called whenever the active account changes.
     * Emits the activeAccountChanged() signal and autosaves the list if enabled.
     */
    void onActiveChanged();

    QList<AccountPtr> m_accounts;

    /*!
     * Account that is currently active.
     */
    AccountPtr m_activeAccount;

    //! Path to the account list file. Empty string if there isn't one.
    QString m_listFilePath;

    /*!
     * If true, the account list will automatically save to the account list path when it changes.
     * Ignored if m_listFilePath is blank.
     */
    bool m_autosave = false;

protected
slots:
    /*!
     * Updates this list with the given list of accounts.
     * This is done by copying each account in the given list and inserting it
     * into this one.
     * We need to do this so that we can set the parents of the accounts are set to this
     * account list. This can't be done in the load task, because the accounts the load
     * task creates are on the load task's thread and Qt won't allow their parents
     * to be set to something created on another thread.
     * To get around that problem, we invoke this method on the GUI thread, which
     * then copies the accounts and sets their parents correctly.
     * \param accounts List of accounts whose parents should be set.
     */
    virtual void updateListData(QList<AccountPtr> versions);
};
