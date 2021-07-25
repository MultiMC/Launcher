#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QPair>
#include <QMap>

#include <memory>
#include "AuthSession.h"
#include "AccountProfile.h"
#include "Usable.h"
#include "providers/BaseAuthProvider.h"

class Task;
class YggdrasilTask;
class Account;

typedef std::shared_ptr<Account> AccountPtr;
Q_DECLARE_METATYPE(AccountPtr)

enum AccountStatus
{
    NotVerified,
    Verified
};

/**
 * Object that stores information about a certain Mojang account.
 *
 * Said information may include things such as that account's username, client token, and access
 * token if the user chose to stay logged in.
 */
class MojangAccount :
    public QObject,
    public Usable,
    public std::enable_shared_from_this<Account>
{
    Q_OBJECT
public: /* construction */
    //! Do not copy accounts. ever.
    explicit Account(const Account &other, QObject *parent) = delete;

    //! Default constructor
    explicit Account(QObject *parent = 0) : QObject(parent) {};

    //! Creates an empty account for the specified user name.
    static AccountPtr createFromUsername(const QString &username);

    //! Loads a Account from the given JSON object.
    static AccountPtr loadFromJson(const QJsonObject &json);

    //! Saves a Account to a JSON object and returns it.
    QJsonObject saveToJson() const;

public: /* manipulation */
        /**
     * Overrides the login type on the account.
     */
    bool setProvider(AuthProviderPtr provider);

    /**
     * Sets the currently selected profile to the profile with the given ID string.
     * If profileId is not in the list of available profiles, the function will simply return
     * false.
     */
    bool setCurrentProfile(const QString &profileId);

    /**
     * Attempt to login. Empty password means we use the token.
     * If the attempt fails because we already are performing some task, it returns false.
     */
    std::shared_ptr<YggdrasilTask> login(AuthSessionPtr session, QString password = QString());
    void invalidateClientToken();

public: /* queries */
    const AuthProviderPtr provider() const
    {
        return m_provider;
    }

    const QString &username() const
    {
        return m_username;
    }

    const QString &clientToken() const
    {
        return m_clientToken;
    }

    const QString &accessToken() const
    {
        return m_accessToken;
    }

    const QList<AccountProfile> &profiles() const
    {
        return m_profiles;
    }

    const User &user()
    {
        return m_user;
    }

    //! Returns the currently selected profile (if none, returns nullptr)
    const AccountProfile *currentProfile() const;

    //! Returns whether the account is NotVerified, Verified or Online
    AccountStatus accountStatus() const;

signals:
    /**
     * This signal is emitted when the account changes
     */
    void changed();

    // TODO: better signalling for the various possible state changes - especially errors

protected: /* variables */
    // Authentication system used.
    // Usable values: "mojang", "dummy", "elyby"
    AuthProviderPtr m_provider;

    // Username taken by account.
    QString m_username;

    // Used to identify the client - the user can have multiple clients for the same account
    // Think: different launchers, all connecting to the same account/profile
    QString m_clientToken;

    // Blank if not logged in.
    QString m_accessToken;

    // Index of the selected profile within the list of available
    // profiles. -1 if nothing is selected.
    int m_currentProfile = -1;

    // List of available profiles.
    QList<AccountProfile> m_profiles;

    // the user structure, whatever it is.
    User m_user;

    // current task we are executing here
    std::shared_ptr<YggdrasilTask> m_currentTask;

protected: /* methods */

    void incrementUses() override;
    void decrementUses() override;

private
slots:
    void authSucceeded();
    void authFailed(QString reason);

private:
    void fillSession(AuthSessionPtr session);

public:
    friend class YggdrasilTask;
    friend class AuthenticateTask;
    friend class ValidateTask;
    friend class RefreshTask;
};
