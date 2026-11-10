#include <QTest>

#include "TestUtil.h"
#include "TestTrees.h"
#include "updater/v2/Plan.h"

using namespace UpdaterV2;

namespace {

TagsDocument tags(qint64 stable, qint64 testing, qint64 latest)
{
    TagsDocument out;
    out.serial = 1;
    out.stable = stable;
    out.testing = testing;
    out.latest = latest;
    return out;
}

BuildManifest build(qint64 number)
{
    BuildManifest out;
    QString error;
    if(!parseBuild(TestTrees::read("main", QString("builds/%1.json").arg(number)), number, out, error))
    {
        qFatal("%s", qPrintable(error));
    }
    return out;
}

SystemRequirements::System mac(const QString &version)
{
    SystemRequirements::System system;
    system.os = "macos";
    system.osVersion = version;
    system.machineArch = "x86_64";
    system.launcherArch = "x86_64";
    system.qt = "5.6.3";
    return system;
}

}

class UpdaterV2PlanTest : public QObject
{
    Q_OBJECT

private
slots:
    void test_tierNames()
    {
        Tier tier;
        QVERIFY(parseTier("latest", tier));
        QVERIFY(tier == Tier::Latest);
        QVERIFY(parseTier("testing", tier));
        QVERIFY(tier == Tier::Testing);
        QVERIFY(parseTier("stable", tier));
        QVERIFY(tier == Tier::Stable);
        // the old channel setting's values aren't tiers
        QVERIFY(!parseTier("develop", tier));
        QVERIFY(!parseTier("Stable", tier));
        QCOMPARE(tierName(Tier::Testing), QString("testing"));
    }

    void test_tiers_data()
    {
        QTest::addColumn<int>("tier");
        QTest::addColumn<qint64>("current");
        QTest::addColumn<bool>("switched");
        QTest::addColumn<int>("kind");
        QTest::addColumn<qint64>("build");
        // stable 4290, testing 4305, latest 4312
        QTest::newRow("stable, older installed") << int(Tier::Stable) << qint64(4280) << false << int(TierDecision::Update) << qint64(4290);
        QTest::newRow("stable, same") << int(Tier::Stable) << qint64(4290) << false << int(TierDecision::NoUpdate) << qint64(4290);
        QTest::newRow("stable, newer installed") << int(Tier::Stable) << qint64(4310) << false << int(TierDecision::BehindInstalled) << qint64(4290);
        QTest::newRow("stable, newer installed, switched") << int(Tier::Stable) << qint64(4310) << true << int(TierDecision::Downgrade) << qint64(4290);
        QTest::newRow("testing") << int(Tier::Testing) << qint64(4290) << false << int(TierDecision::Update) << qint64(4305);
        QTest::newRow("latest") << int(Tier::Latest) << qint64(4290) << false << int(TierDecision::Update) << qint64(4312);
        QTest::newRow("latest, switched, newer") << int(Tier::Latest) << qint64(4290) << true << int(TierDecision::Update) << qint64(4312);
        QTest::newRow("latest, same, switched") << int(Tier::Latest) << qint64(4312) << true << int(TierDecision::NoUpdate) << qint64(4312);
    }
    void test_tiers()
    {
        QFETCH(int, tier);
        QFETCH(qint64, current);
        QFETCH(bool, switched);
        QFETCH(int, kind);
        QFETCH(qint64, build);
        auto decision = decideTier(tags(4290, 4305, 4312), Tier(tier), current, switched);
        QCOMPARE(int(decision.kind), kind);
        QCOMPARE(decision.build, build);
    }

    // right after a promotion there is no newer Testing build: Testing is Stable
    void test_testingResolvesToStable()
    {
        auto decision = decideTier(tags(4305, 4305, 4312), Tier::Testing, 4290, false);
        QCOMPARE(decision.build, qint64(4305));
        // a Testing user ahead of the new Stable stays where they are
        decision = decideTier(tags(4305, 4305, 4312), Tier::Testing, 4310, false);
        QCOMPARE(int(decision.kind), int(TierDecision::BehindInstalled));
    }

    void test_variantFollowsSuccessors()
    {
        // 11 has macos-x86_64-qt5.15, which supersedes qt5.6, from 10.13
        auto choice = chooseVariant(build(11), "macos-x86_64-qt5.6", QString(), mac("10.15.7"));
        QVERIFY(choice.found);
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.15"));
        // too old for the successor: its own variant
        choice = chooseVariant(build(11), "macos-x86_64-qt5.6", QString(), mac("10.12.6"));
        QVERIFY(choice.found);
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.6"));
        // already on the successor
        choice = chooseVariant(build(11), "macos-x86_64-qt5.15", QString(), mac("10.15.7"));
        QVERIFY(choice.found);
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.15"));
        // 10 has no successor yet
        choice = chooseVariant(build(10), "macos-x86_64-qt5.6", QString(), mac("10.15.7"));
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.6"));
    }

    // a successor this Launcher can't read isn't one: it stays on what it can read
    void test_unreadableVariants()
    {
        auto b = build(11);
        auto successor = b.variants.take("macos-x86_64-qt5.15");
        b.unreadable.insert(successor.id, "requires: some later requirement");
        auto choice = chooseVariant(b, "macos-x86_64-qt5.6", QString(), mac("10.15.7"));
        QVERIFY(choice.found);
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.6"));
        // its own variant unreadable: says so, rather than that the build doesn't have it
        choice = chooseVariant(b, "macos-x86_64-qt5.15", QString(), mac("10.15.7"));
        QVERIFY(!choice.found);
        QCOMPARE(choice.unmet.size(), 1);
        QVERIFY2(choice.unmet.first().contains("can't read"), qPrintable(choice.unmet.first()));
    }

    void test_preferredVariant()
    {
        // staying on qt5.6, which 11 still has
        auto choice = chooseVariant(build(11), "macos-x86_64-qt5.6", "macos-x86_64-qt5.6", mac("10.15.7"));
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.6"));
        // going back from qt5.15
        choice = chooseVariant(build(11), "macos-x86_64-qt5.15", "macos-x86_64-qt5.6", mac("10.15.7"));
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.6"));
        // the chosen one doesn't run here: as if none was chosen
        choice = chooseVariant(build(11), "macos-x86_64-qt5.6", "linux-x86_64-qt5", mac("10.15.7"));
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.15"));
        // a build without the chosen one: as if none was chosen
        choice = chooseVariant(build(11), "macos-x86_64-qt5.6", "macos-x86_64-qt5.4", mac("10.15.7"));
        QCOMPARE(choice.variant.id, QString("macos-x86_64-qt5.15"));
    }

    void test_runnableVariants()
    {
        QStringList ids;
        for(auto &v: runnableVariants(build(11), mac("10.15.7")))
        {
            ids << v.id;
        }
        QCOMPARE(ids, QStringList() << "macos-x86_64-qt5.15" << "macos-x86_64-qt5.6");
        QCOMPARE(runnableVariants(build(11), mac("10.6.8")).size(), 0);
    }

    void test_variantNotForThisMachine()
    {
        // the successor isn't met and the old variant needs 10.7
        auto choice = chooseVariant(build(11), "macos-x86_64-qt5.6", QString(), mac("10.6.8"));
        QVERIFY(!choice.found);
        QVERIFY2(choice.unmet.join(" ").contains("10.7"), qPrintable(choice.unmet.join(" ")));

        // on the successor, too old for it, and the build doesn't go back
        choice = chooseVariant(build(11), "macos-x86_64-qt5.15", QString(), mac("10.12"));
        QVERIFY(!choice.found);
        QVERIFY(choice.unmet.join(" ").contains("10.13"));

        auto system = TestTrees::linuxSystem();
        system.glibc = "2.23";
        choice = chooseVariant(build(12), "linux-x86_64-qt5", QString(), system);
        QVERIFY(!choice.found);
        QVERIFY(choice.unmet.join(" ").contains("glibc 2.27"));

        choice = chooseVariant(build(12), "linux-arm64-qt5", QString(), TestTrees::linuxSystem());
        QVERIFY(!choice.found);
        QVERIFY(choice.unmet.join(" ").contains("linux-arm64-qt5"));
    }
};

QTEST_GUILESS_MAIN(UpdaterV2PlanTest)

#include "Plan_test.moc"
