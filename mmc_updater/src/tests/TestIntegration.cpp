#include "TestIntegration.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <functional>

#include "TestUtils.h"
#include <sys/stat.h>

static bool visitFSRecursive(const QString &dir,
							 std::function<bool(const QFileInfo &, const QString &)> visitor,
							 const QString &root = QString())
{
	const auto entries = QDir(dir).entryInfoList(
		QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files, QDir::DirsFirst);
	for (const QFileInfo entry : entries)
	{
		const QString rel =
			QDir(root.isNull() ? dir : root).relativeFilePath(entry.absoluteFilePath());
		if (!visitor(entry, rel))
		{
			return false;
		}
		if (entry.isDir())
		{
			if (!visitFSRecursive(entry.absoluteFilePath(), visitor,
								  root.isNull() ? dir : root))
			{
				return false;
			}
		}
	}
	return true;
}

static int runProcess(const QString &process, const QStringList &arguments)
{
	QProcess proc;
	proc.setProgram(process);
	proc.setArguments(arguments);
	proc.start();
	proc.waitForStarted();
	proc.waitForFinished();
	if (proc.state() != QProcess::NotRunning)
	{
		proc.terminate();
		proc.waitForFinished();
		if (proc.state() != QProcess::NotRunning)
		{
			proc.terminate();
		}
	}
	return proc.exitCode();
}
static QByteArray readFile(const QString &filename)
{
	QFile file(filename);
	if (file.open(QFile::ReadOnly))
	{
		return file.readAll();
	}
	else
	{
		return QByteArray();
	}
}

void TestIntegration::test()
{
	const QString srcPath = QFileInfo(__FILE__).absoluteDir().absoluteFilePath("integration");
	QTemporaryDir workDir;
	const QDir work(workDir.path());
	visitFSRecursive(srcPath, [&work](const QFileInfo &entry, const QString &relativeRoot)
								  -> bool
							  {
		if (entry.isDir())
		{
			work.mkpath(relativeRoot);
		}
		else
		{
			QFile::copy(entry.absoluteFilePath(), work.absoluteFilePath(relativeRoot));
		}
		QFile::setPermissions(work.absoluteFilePath(relativeRoot), entry.permissions());
		return true;
	});

	// TODO finish-cmd, finish-dir and wait
	const int ret =
		runProcess(QDir(qApp->applicationDirPath()).absoluteFilePath("updater"),
				   QStringList() << "--install-dir" << work.absoluteFilePath("installed")
								 << "--package-dir" << work.absoluteFilePath("files")
								 << "--script" << work.absoluteFilePath("file_list.xml")
				   << "--wait" << "21655");
	TEST_COMPARE(ret, 0);

	// check that everything in installed/ exists in result/
	visitFSRecursive(work.absoluteFilePath("installed"),
					 [&work](const QFileInfo &entry, const QString &relativeRoot)
						 -> bool
					 {
		const QString otherPath = work.absoluteFilePath("result/" + relativeRoot);
		TEST_ENSURE(QFileInfo::exists(otherPath));
		TEST_COMPARE(entry.permissions(), QFileInfo(otherPath).permissions());
		if (entry.isDir())
		{
			TEST_ENSURE(QDir(otherPath).exists());
			TEST_ENSURE(QFileInfo(otherPath).isDir());
		}
		else
		{
			TEST_ENSURE(QFile::exists(otherPath));
			TEST_ENSURE(QFileInfo(otherPath).isFile());
			TEST_COMPARE(readFile(entry.absoluteFilePath()), readFile(otherPath));
		}
		return true;
	});
	// check that everything in result/ exists in installed
	visitFSRecursive(work.absoluteFilePath("result"),
					 [&work](const QFileInfo &entry, const QString &relativeRoot)
						 -> bool
					 {
		const QString otherPath = work.absoluteFilePath("installed/" + relativeRoot);
		TEST_ENSURE(QFileInfo::exists(otherPath));
		TEST_COMPARE(entry.permissions(), QFileInfo(otherPath).permissions());
		if (entry.isDir())
		{
			TEST_ENSURE(QDir(otherPath).exists());
			TEST_ENSURE(QFileInfo(otherPath).isDir());
		}
		else
		{
			TEST_ENSURE(QFile::exists(otherPath));
			TEST_ENSURE(QFileInfo(otherPath).isFile());
			TEST_COMPARE(readFile(entry.absoluteFilePath()), readFile(otherPath));
		}
		return true;
	});
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	TestList<TestIntegration> tests;
	tests.addTest(&TestIntegration::test);
	return TestUtils::runTest(tests);
}
