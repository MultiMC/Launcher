#include "FTBUtils.h"

#include <QFile>
#include <QDir>
#include <QRegularExpression>

#include <logger/QsLog.h>

#include "logic/settings/SettingsObject.h"
#include "MultiMC.h"

QString readFtbLaunchCfg(const QString &launcherRoot)
{
	QFile f(QDir(launcherRoot).absoluteFilePath("ftblaunch.cfg"));
	QLOG_INFO() << "Attempting to read" << f.fileName();
	if (!f.open(QFile::ReadOnly))
	{
		QLOG_WARN() << "Couldn't open" << f.fileName() << ":" << f.errorString();
		QLOG_WARN() << "This is perfectly normal if you don't have FTB installed";
		return QString();
	}
	return QString::fromLatin1(f.readAll());
}

QString FTBUtils::getInstallPath(const QString &launcherRoot)
{
	const QString data = readFtbLaunchCfg(launcherRoot);
	QRegularExpression exp("installPath=(.*)");
	QString ftbRoot = QDir::cleanPath(exp.match(data).captured(1));
#ifdef Q_OS_WIN32
	if (!ftbRoot.isEmpty())
	{
		if (ftbRoot.at(0).isLetter() && ftbRoot.size() > 1 && ftbRoot.at(1) == '/')
		{
			ftbRoot.remove(1, 1);
		}
	}
#endif
	if (ftbRoot.isEmpty())
	{
		QLOG_INFO() << "Failed to get FTB root path";
	}
	else
	{
		QLOG_INFO() << "FTB is installed at" << ftbRoot;
	}
	return ftbRoot;
}

FTBUtils::FTBLaunchOptions FTBUtils::getLaunchOptions()
{
	const QString data = readFtbLaunchCfg(MMC->settings()->get("FTBLauncherRoot").toString());
	QRegularExpression ramExp("ramMax=(.*)");
	QRegularExpression extraArgs("additionalJavaOptions=(.*)");
	return FTBLaunchOptions{ramExp.match(data).captured(1), extraArgs.match(data).captured(1)};
}
