#pragma once

#include <launch/LaunchStep.h>
#include <QObjectPtr.h>
#include <LoggedProcess.h>
#include <java/JavaChecker.h>
#include <net/Mode.h>
#include <net/NetJob.h>

struct AuthlibInjector
{
    QString javaArg;

    AuthlibInjector(const QString arg)
    {
        javaArg = std::move(arg);
        qDebug() << "NEW INJECTOR" << javaArg;
    }
};

typedef std::shared_ptr<AuthlibInjector> AuthlibInjectorPtr;

// FIXME: stupid. should be defined by the instance type? or even completely abstracted away...
class InjectAuthlib : public LaunchStep
{
    Q_OBJECT
public:
    InjectAuthlib(LaunchTask *parent, AuthlibInjectorPtr *injector);
    virtual ~InjectAuthlib(){};

    void executeTask() override;
    bool canAbort() const override;
    void proceed() override;

    void setAuthServer(QString server)
    {
        m_authServer = server;
    };

    void setOfflineMode(bool offline) {
        m_offlineMode = offline;
    }

public slots:
    bool abort() override;

private slots:
    void onVersionDownloadSucceeded();
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

private:
    shared_qobject_ptr<Task> jobPtr;
    bool m_aborted = false;

    bool m_offlineMode;
    QString m_versionName;
    QString m_authServer;
    AuthlibInjectorPtr *m_injector;
};
