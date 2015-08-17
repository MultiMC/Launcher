#include <QTest>
#include <QSignalSpy>

#include "tests/TestUtil.h"
#include "logic/updater/UpdateChecker.h"

Q_DECLARE_METATYPE(UpdateChecker::ChannelListEntry)

bool operator==(const UpdateChecker::ChannelListEntry &e1, const UpdateChecker::ChannelListEntry &e2)
{
	return e1.id == e2.id &&
			e1.name == e2.name &&
			e1.description == e2.description &&
			e1.url == e2.url;
}

QDebug operator<<(QDebug dbg, const UpdateChecker::ChannelListEntry &c)
{
	dbg.nospace() << "ChannelListEntry(id=" << c.id << " name=" << c.name << " description=" << c.description << " url=" << c.url << ")";
	return dbg.maybeSpace();
}

class UpdateCheckerTest : public BaseTest
{
	Q_OBJECT
private
slots:
	static QString findTestDataUrl(const char *file)
	{
		return QUrl::fromLocalFile(QFINDTESTDATA(file)).toString();
	}
	void tst_ChannelListParsing_data()
	{
		QTest::addColumn<QString>("channel");
		QTest::addColumn<QString>("channelUrl");
		QTest::addColumn<bool>("hasChannels");
		QTest::addColumn<bool>("valid");
		QTest::addColumn<QList<UpdateChecker::ChannelListEntry> >("result");

		QTest::newRow("garbage")
				<< QString()
				<< findTestDataUrl("tests/data/garbageChannels.json")
				<< false
				<< false
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("errors")
				<< QString()
				<< findTestDataUrl("tests/data/errorChannels.json")
				<< false
				<< true
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("no channels")
				<< QString()
				<< findTestDataUrl("tests/data/noChannels.json")
				<< false
				<< true
				<< QList<UpdateChecker::ChannelListEntry>();
		QTest::newRow("one channel")
				<< QString("develop")
				<< findTestDataUrl("tests/data/oneChannel.json")
				<< true
				<< true
				<< (QList<UpdateChecker::ChannelListEntry>() << UpdateChecker::ChannelListEntry{"develop", "Develop", "The channel called \"develop\"", "http://example.org/stuff"});
		QTest::newRow("several channels")
				<< QString("develop")
				<< findTestDataUrl("tests/data/channels.json")
				<< true
				<< true
				<< (QList<UpdateChecker::ChannelListEntry>()
					<< UpdateChecker::ChannelListEntry{"develop", "Develop", "The channel called \"develop\"", MultiMC_TEST_DATA_PATH}
					<< UpdateChecker::ChannelListEntry{"stable", "Stable", "It's stable at least", MultiMC_TEST_DATA_PATH}
					<< UpdateChecker::ChannelListEntry{"42", "The Channel", "This is the channel that is going to answer all of your questions", "https://dent.me/tea"});
	}
	void tst_ChannelListParsing()
	{

		QFETCH(QString, channel);
		QFETCH(QString, channelUrl);
		QFETCH(bool, hasChannels);
		QFETCH(bool, valid);
		QFETCH(QList<UpdateChecker::ChannelListEntry>, result);

		UpdateChecker checker(channelUrl, channel, 0);

		QSignalSpy channelListLoadedSpy(&checker, SIGNAL(channelListLoaded()));
		QVERIFY(channelListLoadedSpy.isValid());

		checker.updateChanList(false);

		if (valid)
		{
			QVERIFY(channelListLoadedSpy.wait());
			QCOMPARE(channelListLoadedSpy.size(), 1);
		}
		else
		{
			QCOMPARE(channelListLoadedSpy.size(), 0);
		}

		QCOMPARE(checker.hasChannels(), hasChannels);
		QCOMPARE(checker.getChannelList(), result);
	}

	void tst_UpdateChecking()
	{
		QString channel = "develop";
		QString channelUrl = findTestDataUrl("tests/data/channels.json");
		int currentBuild = 2;

		UpdateChecker checker(channelUrl, channel, currentBuild);

		QSignalSpy updateAvailableSpy(&checker, SIGNAL(updateAvailable(GoUpdate::Status)));
		QVERIFY(updateAvailableSpy.isValid());
		QSignalSpy channelListLoadedSpy(&checker, SIGNAL(channelListLoaded()));
		QVERIFY(channelListLoadedSpy.isValid());

		checker.updateChanList(false);
		QVERIFY(channelListLoadedSpy.wait());

		qDebug() << "CWD:" << QDir::current().absolutePath();
		checker.m_channels[0].url = findTestDataUrl("tests/data/");
		checker.checkForUpdate(channel, false);

		QVERIFY(updateAvailableSpy.wait());

		auto status = updateAvailableSpy.first().first().value<GoUpdate::Status>();
		QCOMPARE(checker.m_channels[0].url, status.newRepoUrl);
		QCOMPARE(3, status.newVersionId);
		QCOMPARE(currentBuild, status.currentVersionId);
	}
};

MMCTEST_GUILESS_MAIN(UpdateCheckerTest)

#include "tst_UpdateChecker.moc"
