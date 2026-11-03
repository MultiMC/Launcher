#include <QTest>
#include "TestUtil.h"

#include "meta/UpdateStep.h"

using Meta::UpdateState;
using Meta::UpdateStep;
using Fetch = UpdateState::Fetch;

Q_DECLARE_METATYPE(UpdateState)
Q_DECLARE_METATYPE(UpdateStep)

namespace {
const qint64 Minute = 60 * 1000;

UpdateState root(qint64 fetchedAgo, qint64 failedAgo = -1)
{
    UpdateState state;
    state.isRoot = true;
    state.rootFetchedAgo = fetchedAgo;
    state.rootFailedAgo = failedAgo;
    return state;
}

// a package index or a version, below a root fetched 10 minutes ago
UpdateState child(bool current, bool hasLocal, Fetch fetch = Fetch::NotYet)
{
    UpdateState state;
    state.rootFetchedAgo = 10 * Minute;
    state.current = current;
    state.hasLocal = hasLocal;
    state.fetch = fetch;
    return state;
}

template <typename F>
UpdateState with(UpdateState state, F change)
{
    change(state);
    return state;
}
}

class UpdateStepTest : public QObject
{
    Q_OBJECT
private
slots:
    void test_nextStep_data()
    {
        QTest::addColumn<UpdateState>("state");
        QTest::addColumn<UpdateStep>("step");

        // the root index
        QTest::newRow("root never fetched") << root(-1) << UpdateStep::Fetch;
        QTest::newRow("root fetched 10 minutes ago") << root(10 * Minute) << UpdateStep::Done;
        QTest::newRow("root fetched an hour ago") << root(60 * Minute) << UpdateStep::Fetch;
        QTest::newRow("root fresh, but forced") << with(root(10 * Minute), [](UpdateState &s) { s.forced = true; }) << UpdateStep::Fetch;
        QTest::newRow("root failed a minute ago") << root(60 * Minute, Minute) << UpdateStep::Done;
        QTest::newRow("root failed a minute ago, forced") << with(root(60 * Minute, Minute), [](UpdateState &s) { s.forced = true; }) << UpdateStep::Done;
        QTest::newRow("root failed 6 minutes ago") << root(-1, 6 * Minute) << UpdateStep::Fetch;
        QTest::newRow("root unreachable, local copy") << with(root(-1), [](UpdateState &s) { s.fetch = Fetch::Failed; s.hasLocal = true; }) << UpdateStep::UseLocal;
        QTest::newRow("root unreachable, nothing local") << with(root(-1), [](UpdateState &s) { s.fetch = Fetch::Failed; }) << UpdateStep::Fail;
        QTest::newRow("root refused, local copy") << with(root(-1), [](UpdateState &s) { s.fetch = Fetch::Refused; s.hasLocal = true; }) << UpdateStep::UseLocal;

        // a package index or a version
        QTest::newRow("parent busy") << with(child(false, true), [](UpdateState &s) { s.parentBusy = true; }) << UpdateStep::WaitForParent;
        QTest::newRow("current") << child(true, true) << UpdateStep::Done;
        QTest::newRow("stale") << child(false, true) << UpdateStep::Fetch;
        QTest::newRow("not there") << child(false, false) << UpdateStep::Fetch;
        QTest::newRow("unreachable, local copy") << child(false, true, Fetch::Failed) << UpdateStep::UseLocal;
        QTest::newRow("unreachable, nothing local") << child(false, false, Fetch::Failed) << UpdateStep::Fail;
        QTest::newRow("hash mismatch") << child(false, true, Fetch::Refused) << UpdateStep::RefreshRoot;
        QTest::newRow("hash mismatch, root never fetched") << with(child(false, false, Fetch::Refused), [](UpdateState &s) { s.rootFetchedAgo = -1; }) << UpdateStep::RefreshRoot;
        QTest::newRow("hash mismatch, root being fetched") << with(child(false, true, Fetch::Refused), [](UpdateState &s) { s.rootFetchedAgo = 30 * 1000; s.rootBusy = true; }) << UpdateStep::RefreshRoot;
        QTest::newRow("hash mismatch, root fresh") << with(child(false, true, Fetch::Refused), [](UpdateState &s) { s.rootFetchedAgo = 30 * 1000; }) << UpdateStep::UseLocal;
        QTest::newRow("hash mismatch, root fresh, nothing local") << with(child(false, false, Fetch::Refused), [](UpdateState &s) { s.rootFetchedAgo = 30 * 1000; }) << UpdateStep::Fail;
        QTest::newRow("hash mismatch, root failing") << with(child(false, true, Fetch::Refused), [](UpdateState &s) { s.rootFailedAgo = Minute; }) << UpdateStep::UseLocal;
        QTest::newRow("hash mismatch, root already fetched again") << with(child(false, true, Fetch::Refused), [](UpdateState &s) { s.rootRefreshed = true; }) << UpdateStep::UseLocal;
        QTest::newRow("hash mismatch, root already fetched again, nothing local") << with(child(false, false, Fetch::Refused), [](UpdateState &s) { s.rootRefreshed = true; }) << UpdateStep::Fail;
    }
    void test_nextStep()
    {
        QFETCH(UpdateState, state);
        QFETCH(UpdateStep, step);
        QCOMPARE(int(Meta::nextUpdateStep(state)), int(step));
    }
};

QTEST_GUILESS_MAIN(UpdateStepTest)

#include "UpdateStep_test.moc"
