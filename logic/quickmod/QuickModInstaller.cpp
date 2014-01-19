#include "QuickModInstaller.h"

#include <QMessageBox>
#include <QMimeDatabase>

#include "logic/BaseInstance.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "MultiMC.h"
#include "JlCompress.h"

QuickModInstaller::QuickModInstaller(QWidget *widgetParent, QObject *parent)
	: QObject(parent), m_widgetParent(widgetParent)
{

}

static QDir dirEnsureExists(const QString &dir, const QString &path)
{
	QDir dir_ = QDir::current();

	// first stage
	if (!dir_.exists(dir))
	{
		dir_.mkpath(dir);
	}
	dir_.cd(dir);

	// second stage
	if (!dir_.exists(path))
	{
		dir_.mkpath(path);
	}
	dir_.cd(path);

	return dir_;
}

bool QuickModInstaller::install(const QuickModVersionPtr version, BaseInstance *instance, QString *errorString)
{
	const QString file = MMC->quickmodslist()->existingModFile(version->mod, version);
	QDir finalDir;
	switch (version->installType)
	{
	case QuickModVersion::ForgeMod:
		finalDir = dirEnsureExists(instance->minecraftRoot(), "mods");
		break;
	case QuickModVersion::ForgeCoreMod:
		finalDir = dirEnsureExists(instance->minecraftRoot(), "coremods");
		break;
	case QuickModVersion::Extract:
		finalDir = dirEnsureExists(instance->minecraftRoot(), ".");
		break;
	case QuickModVersion::ConfigPack:
		finalDir = dirEnsureExists(instance->minecraftRoot(), "config");
		break;
	case QuickModVersion::Group:
		return true;
	}

	if (version->installType == QuickModVersion::Extract
			|| version->installType == QuickModVersion::ConfigPack)
	{
		QFileInfo finfo(file);
		// TODO more file formats. KArchive?
		const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(finfo);
		if (mimeType.inherits("application/zip"))
		{
			JlCompress::extractDir(finfo.absoluteFilePath(), finalDir.absolutePath());
		}
		else
		{
			*errorString = tr("Error: Trying to extract an unknown file type %1")
					.arg(finfo.completeSuffix());
			return false;
		}

		return true;
	}
	else
	{
		const QString dest = finalDir.absoluteFilePath(QFileInfo(file).fileName());
		if (QFile::exists(dest))
		{
			if (!QFile::remove(dest))
			{
				QMessageBox::critical(m_widgetParent, tr("Error"),
									  tr("Error deploying %1 to %2").arg(file, dest));
				return false;
			}
		}
		if (!QFile::copy(file, dest))
		{
			QMessageBox::critical(m_widgetParent, tr("Error"),
								  tr("Error deploying %1 to %2").arg(file, dest));
			return false;
		}
		MMC->quickmodslist()->markModAsInstalled(version->mod, version, dest, instance);

		return true;
	}
}
