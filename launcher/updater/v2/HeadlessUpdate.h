#pragma once
#include <QObject>
#include <QString>

#include "Checker.h"
#include "DownloadTask.h"
#include "QObjectPtr.h"

class QNetworkAccessManager;

namespace UpdaterV2
{

/**
 * The --update mode: check a tier, install what it offers, and exit with a status, without
 * any window or question. Made for testing an update before a tag moves (hopper starts the
 * installed build with it, against a copy of the tree), not for users.
 *
 * The tier is checked as if the user had just switched to it in the settings, and what the
 * GUI would then offer is installed: a newer build, a move along `supersedes`, or the tier's
 * older build. Going to Stable from a newer build tests that build's whole updater with the
 * real tree, before any later build exists to update to. The installer restarts the new
 * build with the same arguments plus --alive; with --update, that only writes live.check,
 * stays up for 5 seconds (the installer checks that it runs) and exits (Application), so the
 * next check is the caller's.
 */
class HeadlessUpdate : public QObject
{
    Q_OBJECT
public:
    enum Status
    {
        // the new build is installed and started
        Updated = 0,
        Failed = 1,
        // on the tier's build already, or the tier is behind this build
        NothingToDo = 2,
        // the tier's build has no variant this machine runs
        NotForThisMachine = 3
    };

    HeadlessUpdate(Checker *checker, shared_qobject_ptr<QNetworkAccessManager> network, const QString &site, const QString &root,
                   Tier tier, QObject *parent = nullptr);

    // start; the application exits with a Status when it's done
    void start();

private slots:
    void checked(const UpdaterV2::CheckResult &result);
    void downloaded();
    void downloadFailed(const QString &reason);

private:
    void finish(Status status, const QString &message);

    Checker *m_checker;
    shared_qobject_ptr<QNetworkAccessManager> m_network;
    QString m_site;
    QString m_root;
    Tier m_tier;
    CheckResult m_result;
    std::unique_ptr<DownloadTask> m_download;
};

}
