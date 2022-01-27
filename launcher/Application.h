#pragma once

#include <QApplication>
#include <memory>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QDateTime>
#include <QUrl>
#include <updater/GoUpdate.h>

#include <BaseInstance.h>

#include "minecraft/launch/MinecraftServerTarget.h"

class LaunchController;
class LocalPeer;
class InstanceWindow;
class MainWindow;
class SetupWizard;
class GenericPageProvider;
class QFile;
class HttpMetaCache;
class SettingsObject;
class InstanceList;
class AccountList;
class IconList;
class QNetworkAccessManager;
class JavaInstallList;
class UpdateChecker;
class BaseProfilerFactory;
class BaseDetachedToolFactory;
class TranslationsModel;
class ITheme;
class MCEditTool;
class GAnalytics;

namespace Meta {
    class Index;
}

#if defined(APPLICATION)
#undef APPLICATION
#endif
#define APPLICATION (static_cast<Application *>(QCoreApplication::instance()))

class Application : public QApplication
{
    // friends for the purpose of limiting access to deprecated stuff
    Q_OBJECT
public:
    enum Status {
        StartingUp,
        Failed,
        Succeeded,
        Initialized
    };

public:
    Application(int &argc, char **argv);
    virtual ~Application();

    GAnalytics *analytics() const {
        return m_analytics;
    }

    std::shared_ptr<SettingsObject> settings() const {
        return m_settings;
    }

    qint64 timeSinceStart() const {
        return startTime.msecsTo(QDateTime::currentDateTime());
    }

    QIcon getThemedIcon(const QString& name);

    void setIconTheme(const QString& name);

    std::vector<ITheme *> getValidApplicationThemes();

    void setApplicationTheme(const QString& name, bool initial);

    shared_qobject_ptr<UpdateChecker> updateChecker() {
        return m_updateChecker;
    }

    std::shared_ptr<TranslationsModel> translations();

    std::shared_ptr<JavaInstallList> javalist();

    std::shared_ptr<InstanceList> instances() const {
        return m_instances;
    }

    std::shared_ptr<IconList> icons() const {
        return m_icons;
    }

    MCEditTool *mcedit() const {
        return m_mcedit.get();
    }

    shared_qobject_ptr<AccountList> accounts() const {
        return m_accounts;
    }

    QString msaClientId() const;

    Status status() const {
        return m_status;
    }

    const QMap<QString, std::shared_ptr<BaseProfilerFactory>> &profilers() const {
        return m_profilers;
    }

    void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

    shared_qobject_ptr<QNetworkAccessManager> network();

    shared_qobject_ptr<HttpMetaCache> metacache();

    shared_qobject_ptr<Meta::Index> metadataIndex();

    QString getJarsPath();

    /// this is the root of the 'installation'. Used for automatic updates
    const QString &root() {
        return m_rootPath;
    }

    /*!
     * Opens a json file using either a system default editor, or, if not empty, the editor
     * specified in the settings
     */
    bool openJsonEditor(const QString &filename);

    InstanceWindow *showInstanceWindow(InstancePtr instance, QString page = QString());
    MainWindow *showMainWindow(bool minimized = false);

    void updateIsRunning(bool running);
    bool updatesAreAllowed();

    void ShowGlobalSettings(class QWidget * parent, QString open_page = QString());

signals:
    void updateAllowedChanged(bool status);
    void globalSettingsAboutToOpen();
    void globalSettingsClosed();

public slots:
    bool launch(
        InstancePtr instance,
        bool online = true,
        BaseProfilerFactory *profiler = nullptr,
        MinecraftServerTargetPtr serverToJoin = nullptr,
        MinecraftAccountPtr accountToUse = nullptr
    );
    bool kill(InstancePtr instance);

private slots:
    void on_windowClose();
    void messageReceived(const QByteArray & message);
    void controllerSucceeded();
    void controllerFailed(const QString & error);
    void analyticsSettingChanged(const Setting &setting, QVariant value);
    void setupWizardFinished(int status);

private:
    bool createSetupWizard();
    void performMainStartupAction();

    // sets the fatal error message and m_status to Failed.
    void showFatalErrorMessage(const QString & title, const QString & content);

private:
    void addRunningInstance();
    void subRunningInstance();
    bool shouldExitNow() const;

private:
    QDateTime startTime;

    shared_qobject_ptr<QNetworkAccessManager> m_network;

    shared_qobject_ptr<UpdateChecker> m_updateChecker;
    shared_qobject_ptr<AccountList> m_accounts;

    shared_qobject_ptr<HttpMetaCache> m_metacache;
    shared_qobject_ptr<Meta::Index> m_metadataIndex;

    std::shared_ptr<SettingsObject> m_settings;
    std::shared_ptr<InstanceList> m_instances;
    std::shared_ptr<IconList> m_icons;
    std::shared_ptr<JavaInstallList> m_javalist;
    std::shared_ptr<TranslationsModel> m_translations;
    std::shared_ptr<GenericPageProvider> m_globalSettingsProvider;
    std::map<QString, std::unique_ptr<ITheme>> m_themes;
    std::unique_ptr<MCEditTool> m_mcedit;
    QString m_jarsPath;
    QSet<QString> m_features;

    QMap<QString, std::shared_ptr<BaseProfilerFactory>> m_profilers;

    QString m_rootPath;
    Status m_status = Application::StartingUp;

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
#endif

    // FIXME: attach to instances instead.
    struct InstanceXtras {
        InstanceWindow * window = nullptr;
        shared_qobject_ptr<LaunchController> controller;
    };
    std::map<QString, InstanceXtras> m_instanceExtras;

    // main state variables
    size_t m_openWindows = 0;
    size_t m_runningInstances = 0;
    bool m_updateRunning = false;

    // main window, if any
    MainWindow * m_mainWindow = nullptr;

    // peer launcher instance connector - used to implement single instance launcher and signalling
    LocalPeer * m_peerInstance = nullptr;

    GAnalytics * m_analytics = nullptr;
    SetupWizard * m_setupWizard = nullptr;
public:
    QString m_instanceIdToLaunch;
    QString m_serverToJoin;
    QString m_profileToUse;
    bool m_liveCheck = false;
    QUrl m_zipToImport;
    std::unique_ptr<QFile> logFile;
};
