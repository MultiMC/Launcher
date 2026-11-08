#include <QTest>
#include <QElapsedTimer>
#include <QTemporaryDir>

#include "TestUtil.h"
#include "TestTrees.h"
#include "updater/v2/Checker.h"
#include "updater/v2/DownloadTask.h"
#include "FileSystem.h"
#include "net/Mirrors.h"

using namespace UpdaterV2;

class UpdaterV2CheckerTest : public QObject
{
    Q_OBJECT

    shared_qobject_ptr<QNetworkAccessManager> m_network;
    std::unique_ptr<QTemporaryDir> m_dir;

    QString siteUrl(const QString &tree)
    {
        // a site with nothing on it
        if(tree == "nowhere")
        {
            return QUrl::fromLocalFile(m_dir->path() + "/nowhere/").toString();
        }
        return TestTrees::url(tree);
    }

    // a Launcher on build 10 of linux-x86_64-qt5, starting from the trees' floor, with the
    // files sites of `trees` compiled in as the files site's base URLs
    Config config(const QStringList &trees, qint64 currentBuild = 10)
    {
        Config out;
        QStringList sites;
        for(auto &tree: trees)
        {
            sites.append(siteUrl(tree));
        }
        Net::Mirrors::setSites({sites});
        out.site = sites.first();
        out.rootKeys = TestTrees::rootKeys();
        out.floorKeys = TestTrees::floor();
        out.variant = "linux-x86_64-qt5";
        out.currentBuild = currentBuild;
        out.statePath = m_dir->path() + "/updater/state.json";
        out.system = TestTrees::linuxSystem();
        out.now = QDateTime(QDate(2026, 11, 1), QTime(12, 0), Qt::UTC);
        return out;
    }

    CheckResult run(const Config &config, Tier tier, bool tierSwitched = false, const QString &preferred = QString())
    {
        Checker checker(m_network, config);
        checker.setPreferredVariant(preferred);
        CheckResult out;
        bool done = false;
        connect(&checker, &Checker::finished, [&](const CheckResult &result)
        {
            out = result;
            done = true;
        });
        checker.check(tier, tierSwitched, true);
        QElapsedTimer timer;
        timer.start();
        while(!done && timer.elapsed() < 20000)
        {
            QTest::qWait(10);
        }
        if(!done)
        {
            qWarning() << "the check didn't finish";
        }
        return out;
    }

private
slots:
    void init()
    {
        m_network.reset(new QNetworkAccessManager());
        m_dir.reset(new QTemporaryDir());
    }

    void test_noUpdate()
    {
        auto result = run(config({"main"}), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::NoUpdate));
        QCOMPARE(result.build, qint64(10));
        QVERIFY(result.notify);
        QVERIFY(!result.currentWithdrawn);

        // what was verified is kept
        State state(TestTrees::floor(), TestTrees::rootKeys(), 0);
        state.load(m_dir->path() + "/updater/state.json");
        QCOMPARE(state.serial(), qint64(7));
        QCOMPARE(state.keys().version, qint64(2));
        QVERIFY(state.hasMirrors());
    }

    void test_update_data()
    {
        QTest::addColumn<int>("tier");
        QTest::addColumn<qint64>("build");
        QTest::newRow("latest") << int(Tier::Latest) << qint64(12);
        QTest::newRow("testing") << int(Tier::Testing) << qint64(11);
    }
    void test_update()
    {
        QFETCH(int, tier);
        QFETCH(qint64, build);
        auto result = run(config({"main"}), Tier(tier));
        QCOMPARE(int(result.kind), int(CheckResult::Update));
        QCOMPARE(result.build, build);
        QCOMPARE(result.variant.id, QString("linux-x86_64-qt5"));
        QCOMPARE(result.commit, QString("%1").arg(0xc0ffee + build, 40, 16, QChar('0')));
        // build 10's list, for what to remove
        QVERIFY(result.haveCurrent);
        QCOMPARE(result.current.files.last().path, QString("share/old.txt"));
    }

    void test_behindAndDowngrade()
    {
        auto result = run(config({"main"}, 11), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::BehindInstalled));
        QCOMPARE(result.build, qint64(10));

        result = run(config({"main"}, 11), Tier::Stable, true);
        QCOMPARE(int(result.kind), int(CheckResult::Downgrade));
        QCOMPARE(result.build, qint64(10));
        QVERIFY(result.tierSwitched);
        QCOMPARE(result.current.files.last().path, QString("share/new.txt"));
    }

    void test_withdrawnBuild()
    {
        // build 9 is withdrawn and its manifest is gone: nothing to remove is known
        auto result = run(config({"main"}, 9), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Update));
        QVERIFY(result.currentWithdrawn);
        QVERIFY(!result.haveCurrent);
        QVERIFY(result.messages.join(" ").contains("withdrawn"));
    }

    void test_notForThisMachine()
    {
        auto cfg = config({"main"});
        cfg.system.glibc = "2.23";
        auto result = run(cfg, Tier::Latest);
        QCOMPARE(int(result.kind), int(CheckResult::NotForThisMachine));
        QCOMPARE(result.build, qint64(12));
        QVERIFY(result.messages.join(" ").contains("glibc 2.27"));
        // Testing still works there
        QCOMPARE(int(run(cfg, Tier::Testing).kind), int(CheckResult::Update));
    }

    // a variant with links, superseding the installed one
    void test_links()
    {
        auto cfg = config({"main"});
        cfg.variant = "macos-x86_64-qt5.6";
        cfg.system.os = "macos";
        cfg.system.osVersion = "10.15.7";
        auto result = run(cfg, Tier::Testing);
        QCOMPARE(int(result.kind), int(CheckResult::Update));
        QCOMPARE(result.variant.id, QString("macos-x86_64-qt5.15"));
        QCOMPARE(result.variant.links.size(), 2);
        QCOMPARE(result.current.id, QString("macos-x86_64-qt5.6"));
    }

    // on the tier's build in a superseded variant: the successor is offered without a newer build
    void test_switchOnTheSameBuild()
    {
        auto cfg = config({"main"}, 11);
        cfg.variant = "macos-x86_64-qt5.6";
        cfg.system.os = "macos";
        cfg.system.osVersion = "10.15.7";
        auto result = run(cfg, Tier::Testing);
        QCOMPARE(int(result.kind), int(CheckResult::Switch));
        QCOMPARE(result.build, qint64(11));
        QCOMPARE(result.variant.id, QString("macos-x86_64-qt5.15"));
        QVERIFY(result.haveCurrent);
        QCOMPARE(result.current.id, QString("macos-x86_64-qt5.6"));
        QCOMPARE(result.recommended, QString("macos-x86_64-qt5.15"));
        QCOMPARE(result.runnable.size(), 2);
        QVERIFY(!result.ownVariantDropped);

        // the user stays on it
        result = run(cfg, Tier::Testing, false, "macos-x86_64-qt5.6");
        QCOMPARE(int(result.kind), int(CheckResult::NoUpdate));

        // and goes back to it from the successor
        cfg.variant = "macos-x86_64-qt5.15";
        result = run(cfg, Tier::Testing, true, "macos-x86_64-qt5.6");
        QCOMPARE(int(result.kind), int(CheckResult::Switch));
        QCOMPARE(result.variant.id, QString("macos-x86_64-qt5.6"));
        QCOMPARE(result.current.id, QString("macos-x86_64-qt5.15"));
    }

    // a build without this variant: said so, with what runs here
    void test_ownVariantDropped()
    {
        auto cfg = config({"main"});
        cfg.variant = "linux-x86-qt5";
        auto result = run(cfg, Tier::Latest);
        QCOMPARE(int(result.kind), int(CheckResult::NotForThisMachine));
        QVERIFY(result.ownVariantDropped);
        QCOMPARE(result.runnable.size(), 1);
    }

    // the first mirror's tags don't verify: they come from the next one
    void test_fallback()
    {
        auto result = run(config({"broken", "main"}), Tier::Latest);
        QCOMPARE(int(result.kind), int(CheckResult::Update));
        QCOMPARE(result.build, qint64(12));

        // not there at all
        result = run(config({"nowhere", "main"}), Tier::Latest);
        QCOMPARE(int(result.kind), int(CheckResult::Update));
    }

    void test_allMirrorsFail()
    {
        auto result = run(config({"broken"}), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Failed));
        QVERIFY2(result.messages.join(" ").startsWith("tags.json: "), qPrintable(result.messages.join(" ")));
        QVERIFY2(result.messages.join(" ").contains("the signature by"), qPrintable(result.messages.join(" ")));
    }

    void test_mirrorListIsUsed()
    {
        auto cfg = config({"main"});
        run(cfg, Tier::Stable);
        auto url = QUrl(cfg.site + "translations/index");
        // the list's .invalid hosts, which couldn't be reached and so come last now, and the compiled-in site
        auto candidates = Net::Mirrors::candidates(url);
        QCOMPARE(candidates.size(), 4);
        QCOMPARE(candidates.first(), url);
        QVERIFY(candidates.contains(QUrl("https://files.invalid/translations/index")));

        // a Launcher starting with the stored list uses it from the start; the list's .invalid
        // hosts fail, and the compiled-in site after them answers
        cfg = config({"main"});
        QCOMPARE(Net::Mirrors::candidates(url).size(), 1);
        Checker checker(m_network, cfg);
        QCOMPARE(Net::Mirrors::candidates(url).size(), 4);
        QCOMPARE(int(run(cfg, Tier::Stable).kind), int(CheckResult::NoUpdate));
    }

    // a mirror that lags behind serves an older (validly signed) tags document
    void test_lowerSerialRefused()
    {
        QCOMPARE(int(run(config({"stale"}), Tier::Stable).kind), int(CheckResult::NoUpdate));
        QCOMPARE(int(run(config({"main"}), Tier::Stable).kind), int(CheckResult::NoUpdate));
        // now serial 7 and keys version 2 are known
        auto result = run(config({"stale", "main"}), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::NoUpdate));

        result = run(config({"stale"}), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Failed));
        // stale's documents are signed by online1, which keys version 2 dropped
        QVERIFY2(result.messages.join(" ").contains("no signature by a trusted key"), qPrintable(result.messages.join(" ")));
    }

    void test_serialFloor()
    {
        auto cfg = config({"main"});
        cfg.serialFloor = 8;
        auto result = run(cfg, Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Failed));
        QVERIFY(result.messages.join(" ").contains("serial 7 is lower than 8"));
    }

    void test_rootRotation()
    {
        auto result = run(config({"rotated"}), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::NoUpdate));
        State state(TestTrees::floor(), TestTrees::rootKeys(), 0);
        state.load(m_dir->path() + "/updater/state.json");
        QCOMPARE(state.keys().version, qint64(3));
        QCOMPARE(state.serial(), qint64(8));

        // from keys version 3, main's documents (signed by online2) aren't trusted
        result = run(config({"main"}), Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Failed));
    }

    void test_expiredKeys()
    {
        auto cfg = config({"main"});
        cfg.now = QDateTime(QDate(2028, 11, 1), QTime(0, 0), Qt::UTC);
        auto result = run(cfg, Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Failed));
        QVERIFY(result.messages.join(" ").contains("expired"));

        // the intermediate version 1 expired already, which doesn't matter
        cfg.now = QDateTime(QDate(2027, 6, 1), QTime(0, 0), Qt::UTC);
        QCOMPARE(int(run(cfg, Tier::Stable).kind), int(CheckResult::NoUpdate));
    }

    void test_wrongPinnedKeys()
    {
        auto cfg = config({"main"});
        cfg.rootKeys = QStringList() << QString(64, 'a');
        Checker checker(m_network, cfg);
        QVERIFY(!checker.configError().isEmpty());
        auto result = run(cfg, Tier::Stable);
        QCOMPARE(int(result.kind), int(CheckResult::Failed));
        QVERIFY(result.messages.join(" ").contains("pinned root keys"));
    }

    void test_queuedCheck()
    {
        Checker checker(m_network, config({"main"}, 11));
        QList<CheckResult> results;
        connect(&checker, &Checker::finished, [&](const CheckResult &result) { results.append(result); });
        checker.check(Tier::Stable, false, false);
        checker.check(Tier::Stable, true, true);
        QTRY_COMPARE_WITH_TIMEOUT(results.size(), 2, 20000);
        QCOMPARE(int(results[0].kind), int(CheckResult::BehindInstalled));
        QCOMPARE(int(results[1].kind), int(CheckResult::Downgrade));
        QVERIFY(results[1].notify);
    }

    /*
     * A real tree, only when asked for (it needs the network): check from build 1 on this
     * machine's own system, then download everything into an empty installation.
     *   TREE_UPDATER_LIVE_SITES    the files site's base URLs, separated by ;
     *   TREE_UPDATER_LIVE_FLOOR    the floor keys document (its root keys are pinned)
     *   TREE_UPDATER_LIVE_VARIANT  the variant ID to check for
     */
    void test_liveTree()
    {
        auto sites = QString::fromLocal8Bit(qgetenv("TREE_UPDATER_LIVE_SITES")).split(';', QString::SkipEmptyParts);
        if(sites.isEmpty())
        {
            QSKIP("TREE_UPDATER_LIVE_SITES isn't set");
        }
        QFile floorFile(QString::fromLocal8Bit(qgetenv("TREE_UPDATER_LIVE_FLOOR")));
        QVERIFY(floorFile.open(QFile::ReadOnly));
        auto floor = floorFile.readAll();

        Net::Mirrors::setSites({sites});
        Config cfg;
        cfg.site = sites.first();
        for(auto key: QJsonDocument::fromJson(floor).object().value("root_keys").toArray())
        {
            cfg.rootKeys.append(key.toString());
        }
        cfg.floorKeys = floor;
        cfg.variant = QString::fromLocal8Bit(qgetenv("TREE_UPDATER_LIVE_VARIANT"));
        cfg.currentBuild = 1;
        cfg.statePath = m_dir->path() + "/updater/state.json";
        cfg.system = SystemRequirements::detect();
        auto result = run(cfg, Tier::Latest);
        qDebug() << "live tree:" << result.kind << "build" << result.build << "variant" << result.variant.id << result.messages;
        QCOMPARE(int(result.kind), int(CheckResult::Update));
        QVERIFY(!result.haveCurrent);

        auto staging = m_dir->path() + "/update/XXXXXX";
        FS::ensureFilePathExists(staging);
        UpdaterV2::DownloadTask task(m_network, result, cfg.site, m_dir->path() + "/root", staging);
        bool done = false, succeeded = false;
        connect(&task, &Task::succeeded, [&]() { done = succeeded = true; });
        connect(&task, &Task::failed, [&](QString) { done = true; });
        task.start();
        QTRY_VERIFY_WITH_TIMEOUT(done, 300000);
        QVERIFY2(succeeded, qPrintable(task.failReason()));
        QCOMPARE(task.changes().size(), result.variant.files.size() + result.variant.links.size());
        qDebug() << "live tree: downloaded" << result.variant.files.size() << "files";
    }
};

QTEST_GUILESS_MAIN(UpdaterV2CheckerTest)

#include "Checker_test.moc"
