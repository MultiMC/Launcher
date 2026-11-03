#include <QTest>
#include <QElapsedTimer>
#include <QTemporaryDir>

#include "TestUtil.h"
#include "TestTrees.h"
#include "FileSystem.h"
#include "net/Mirrors.h"
#include "updater/v2/DownloadTask.h"

using namespace UpdaterV2;

namespace {

Variant variant(qint64 build)
{
    BuildManifest out;
    QString error;
    parseBuild(TestTrees::read("main", QString("builds/%1.json").arg(build)), build, out, error);
    return out.variants.value("linux-x86_64-qt5");
}

void writeFile(const QString &path, const QByteArray &data)
{
    FS::ensureFilePathExists(path);
    QFile file(path);
    file.open(QFile::WriteOnly);
    file.write(data);
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    file.open(QFile::ReadOnly);
    return file.readAll();
}

}

class UpdaterV2DownloadTaskTest : public QObject
{
    Q_OBJECT

    shared_qobject_ptr<QNetworkAccessManager> m_network;
    std::unique_ptr<QTemporaryDir> m_dir;

    QString root()
    {
        return m_dir->path() + "/root";
    }

    // as MainWindow makes it
    QString staging()
    {
        auto path = m_dir->path() + "/update/XXXXXX";
        FS::ensureFilePathExists(path);
        return path;
    }

    // a files site with a tree in it that serves something else than the manifest lists
    QString badSite(const QByteArray &content)
    {
        auto site = m_dir->path() + "/bad/";
        writeFile(site + "updater/" + contentPath(variant(11).files[0].sha256), content);
        return QUrl::fromLocalFile(site).toString();
    }

    // build 10 installed, an update to 11 found
    CheckResult update()
    {
        for(auto &file: variant(10).files)
        {
            auto path = FS::PathCombine(root(), file.path);
            writeFile(path, TestTrees::read("main", contentPath(file.sha256)));
            if(file.executable)
            {
                QFile::setPermissions(path, QFile::permissions(path) | QFile::ExeOwner | QFile::ExeUser);
            }
        }
        CheckResult out;
        out.kind = CheckResult::Update;
        out.currentBuild = 10;
        out.build = 11;
        out.variant = variant(11);
        out.haveCurrent = true;
        out.current = variant(10);
        return out;
    }

    bool run(DownloadTask &task)
    {
        bool done = false, succeeded = false;
        connect(&task, &Task::succeeded, [&]() { done = succeeded = true; });
        connect(&task, &Task::failed, [&](QString) { done = true; });
        task.start();
        QElapsedTimer timer;
        timer.start();
        while(!done && timer.elapsed() < 20000)
        {
            QTest::qWait(10);
        }
        return succeeded;
    }

private
slots:
    void init()
    {
        m_network.reset(new QNetworkAccessManager());
        m_dir.reset(new QTemporaryDir());
    }

    void test_download()
    {
        Net::Mirrors::setSites({{TestTrees::url("main")}});
        DownloadTask task(m_network, update(), TestTrees::url("main"), root(), staging());
        QVERIFY(run(task));
        QStringList summary;
        for(auto &change: task.changes())
        {
            summary << QString("%1 %2").arg(change.kind).arg(change.path);
            if(change.kind == Change::Place)
            {
                QVERIFY(change.source.startsWith(task.stagingDir()));
            }
        }
        // bin/libfoo.so is the same in both builds
        QCOMPARE(summary, QStringList() << "2 share/old.txt" << "0 MultiMC" << "0 bin/MultiMC" << "0 share/new.txt");
        auto next = variant(11);
        QCOMPARE(next.files[1].path, QString("bin/MultiMC"));
        QCOMPARE(readFile(task.changes()[2].source), TestTrees::read("main", contentPath(next.files[1].sha256)));
    }

    void test_upToDate()
    {
        Net::Mirrors::setSites({{TestTrees::url("main")}});
        auto check = update();
        check.variant = variant(10);
        DownloadTask task(m_network, check, TestTrees::url("main"), root(), staging());
        QVERIFY(run(task));
        QVERIFY(task.changes().isEmpty());
    }

    // a mirror serving a wrong file: it comes from the next one
    void test_badContentFallsBack()
    {
        auto bad = badSite("not what the manifest lists\n");
        Net::Mirrors::setSites({{bad, TestTrees::url("main")}});
        DownloadTask task(m_network, update(), bad, root(), staging());
        QVERIFY(run(task));
        QCOMPARE(readFile(task.changes()[1].source), TestTrees::read("main", contentPath(variant(11).files[0].sha256)));
    }

    void test_allMirrorsFail()
    {
        // more than the manifest says
        auto bad = badSite(TestTrees::read("main", contentPath(variant(11).files[0].sha256)) + "x");
        Net::Mirrors::setSites({{bad}});
        DownloadTask task(m_network, update(), bad, root(), staging());
        QVERIFY(!run(task));
        QVERIFY(task.changes().isEmpty());
    }
};

QTEST_GUILESS_MAIN(UpdaterV2DownloadTaskTest)

#include "DownloadTask_test.moc"
