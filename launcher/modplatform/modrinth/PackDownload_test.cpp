// SPDX-License-Identifier: Apache-2.0
#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include "net/NetJob.h"
#include "net/ChecksumValidator.h"
#include "PackUpdate.h"
#include "ui/dialogs/PackUpdateDialog.h"
#include <atomic>
#include <QLockFile>
#include <QSysInfo>
#include "NullInstance.h"
#include "InstanceCopyTask.h"
#include "settings/INISettingsObject.h"

namespace {
SettingsObjectPtr globals(const QString &root)
{
    auto settings = std::make_shared<INISettingsObject>(root + "/global.cfg");
    for (auto key : {"PreLaunchCommand", "WrapperCommand", "PostExitCommand", "ShowConsole",
                     "AutoCloseConsole", "ShowConsoleOnError", "LogPrePostOutput", "ConsoleMaxLines", "ConsoleOverflowStop"})
        settings->registerSetting(key, "");
    return settings;
}
void writeFixture(const QString &path, const QByteArray &bytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) qFatal("Cannot write fixture");
}
}


class PackDownloadTest : public QObject
{
    Q_OBJECT
private slots:
    void launchLocks()
    {
        QTemporaryDir temp;
        auto root = temp.path();
        auto global = globals(root);
        auto settings = std::make_shared<INISettingsObject>(root + "/instance.cfg");
        NullInstance instance(global, settings, root);
        instance.setVersionBroken(false);
        QVERIFY(instance.BaseInstance::canLaunch());
        instance.setPackUpdateBusy(true);
        QVERIFY(!instance.BaseInstance::canLaunch());
        instance.setPackUpdateBusy(false);
        QDir().mkpath(root + "/.pack-update");
        QLockFile lock(root + "/.pack-update/lock");
        QVERIFY(lock.tryLock());
        QVERIFY(!instance.BaseInstance::canLaunch());
        lock.unlock();
        writeFixture(root + "/.pack-update/lock", "2147483647\nMultiMC\n" + QSysInfo::machineHostName().toUtf8() + "\n");
        QVERIFY(instance.BaseInstance::canLaunch());
        writeFixture(root + "/.pack-update/journal.json", "{}");
        QVERIFY(!instance.BaseInstance::canLaunch());
    }
    void copyExcludesPrivateBackup()
    {
        QTemporaryDir temp;
        auto global = globals(temp.path());
        const auto source = temp.path() + "/source";
        const auto target = temp.path() + "/copy";
        writeFixture(source + "/.minecraft/saves/world/data", "private world");
        writeFixture(source + "/.pack-update/backup/.minecraft/saves/world/data", "backup world");
        writeFixture(source + "/.pack-update/lock", "lock");
        writeFixture(source + "/modrinth-update.json", "baseline");
        writeFixture(source + "/.minecraft/options.txt", "keys");
        InstancePtr instance(new NullInstance(global, std::make_shared<INISettingsObject>(source + "/instance.cfg"), source));
        InstanceCopyTask task(instance, false, true);
        task.setParentSettings(global);
        task.setStagingPath(target);
        task.setName("Copy");
        QSignalSpy finished(&task, &Task::finished);
        task.start();
        QVERIFY(finished.wait(5000));
        QVERIFY(task.wasSuccessful());
        QVERIFY(!QFileInfo::exists(target + "/.pack-update"));
        QVERIFY(!QFileInfo::exists(target + "/.minecraft/saves"));
        QVERIFY(QFileInfo::exists(target + "/modrinth-update.json"));
        QVERIFY(QFileInfo::exists(target + "/.minecraft/options.txt"));
    }
    void workerWaitsDuringShutdown()
    {
        std::atomic<bool> completed(false);
        {
            PackUpdateWork task("Shutdown fixture", [&]() {
                QThread::msleep(20);
                completed.store(true);
            });
            task.start();
            // Destruction can happen when the application event loop exits.
        }
        QVERIFY(completed.load());
    }
    void verifiedDownload_data()
    {
        QTest::addColumn<int>("mode");
        QTest::newRow("verified") << 0;
        QTest::newRow("bad-checksum") << 1;
        QTest::newRow("http-failure") << 2;
        QTest::newRow("cancel") << 3;
    }
    void verifiedDownload()
    {
        QFETCH(int, mode);
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray payload("verified mod payload");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            while (server.hasPendingConnections()) {
                auto socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket, [socket, mode, payload]() {
                    socket->readAll();
                    if (mode == 3) return;
                    auto response = mode == 2 ? QByteArray("HTTP/1.1 503 Unavailable\r\n") : QByteArray("HTTP/1.1 200 OK\r\n");
                    socket->write(response + "Content-Length: " + QByteArray::number(payload.size()) + "\r\nConnection: close\r\n\r\n" + payload);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
            }
        });
        QTemporaryDir temp;
        auto path = temp.path() + "/download.jar";
        shared_qobject_ptr<QNetworkAccessManager> network = new QNetworkAccessManager();
        NetJob job("Fixture download", network);
        auto action = Net::Download::makeFile(QUrl(QString("http://127.0.0.1:%1/mod.jar").arg(server.serverPort())), path);
        auto checksum = QCryptographicHash::hash(mode == 1 ? QByteArray("wrong") : payload, QCryptographicHash::Sha512);
        action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512, checksum));
        job.addNetAction(action);
        QSignalSpy finished(&job, &Task::finished);
        job.start();
        if (mode == 3) QTimer::singleShot(20, &job, [&]() { job.abort(); });
        QVERIFY(finished.wait(15000));
        QCOMPARE(job.wasSuccessful(), mode == 0);
        if (mode == 0) QCOMPARE(Modrinth::Update::digest(path), QString::fromLatin1(checksum.toHex()));
        else QVERIFY(!QFileInfo::exists(path));
    }
};
QTEST_GUILESS_MAIN(PackDownloadTest)
#include "PackDownload_test.moc"
