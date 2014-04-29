#include "QuickModInstaller.h"

#include <QMessageBox>
#include <QMimeDatabase>

#include "logic/BaseInstance.h"
#include "logic/OneSixInstance.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickMod.h"
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
static QString fileName(const QuickModVersionPtr &version, const QUrl &url)
{
	QString ending = QMimeDatabase().mimeTypeForUrl(url).preferredSuffix();
	if (!ending.isEmpty())
	{
		ending.prepend('.');
	}
	if (ending == ".bin")
	{
		ending = ".jar";
	}
	return version->mod->uid() + "-" + version->name() + ending;
}

void QuickModInstaller::install(const QuickModVersionPtr version, InstancePtr instance)
{
	QMap<QString, QString> otherVersions = MMC->quickmodslist()->installedModFiles(version->mod, instance);
	for (auto it = otherVersions.begin(); it != otherVersions.end(); ++it)
	{
		if (!QFile::remove(it.value()))
		{
			QLOG_ERROR() << "Unable to remove previous version file" << it.value() << ", this may cause problems";
		}
		MMC->quickmodslist()->markModAsUninstalled(version->mod, version->mod->version(it.key()), instance);
	}

	const QString file = MMC->quickmodslist()->existingModFile(version->mod, version);
	QDir finalDir;
	switch (version->installType)
	{
	case QuickModVersion::ForgeMod:
		// FIXME: This will break if the type name changes.
		if (instance->instanceType() == "OneSix")
		{
			return;
		}
		finalDir = dirEnsureExists(instance->minecraftRoot(), "mods");
		break;
	case QuickModVersion::ForgeCoreMod:
		if (instance->instanceType() == "OneSix")
		{
			finalDir = dirEnsureExists(instance->minecraftRoot(), "mods");
		}
		else
		{
			finalDir = dirEnsureExists(instance->minecraftRoot(), "coremods");
		}
		break;
	case QuickModVersion::Extract:
		finalDir = dirEnsureExists(instance->minecraftRoot(), ".");
		break;
	case QuickModVersion::ConfigPack:
		finalDir = dirEnsureExists(instance->minecraftRoot(), "config");
		break;
	case QuickModVersion::Group:
		return;
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
			throw new MMCError(tr("Error: Trying to extract an unknown file type %1").arg(finfo.completeSuffix()));
		}
	}
	else
	{
		const QString dest = finalDir.absoluteFilePath(QFileInfo(file).fileName());
		if (QFile::exists(dest))
		{
			if (!QFile::remove(dest))
			{
				throw new MMCError(tr("Error: Deploying %1 to %2").arg(file, dest));
			}
		}
		if (!QFile::copy(file, dest))
		{
			throw new MMCError(tr("Error: Deploying %1 to %2").arg(file, dest));
		}
		MMC->quickmodslist()->markModAsInstalled(version->mod, version, dest, instance);
	}
}

void QuickModInstaller::handleDownload(QuickModVersionPtr version, const QByteArray &data, const QUrl &url)
{
	QDir dir;
	switch (version->installType)
	{
	case QuickModVersion::ForgeMod:
	case QuickModVersion::ForgeCoreMod:
		dir = dirEnsureExists(MMC->settings()->get("CentralModsDir").toString(), "mods");
		break;
	case QuickModVersion::Extract:
	case QuickModVersion::ConfigPack:
		dir = dirEnsureExists(MMC->settings()->get("CentralModsDir").toString(), "archives");
		break;
	case QuickModVersion::Group:
		return;
	}

	const QByteArray actual = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
	if (!version->md5.isNull() && actual != version->md5)
	{
		QLOG_INFO() << "Checksum missmatch for " << version->mod->uid()
					<< ". Actual: " << actual << " Expected: " << version->md5;
		// FIXME using exceptions generates crashes?
		throw new MMCError(tr("Error: Checksum mismatch"));
	}

	QFile file(dir.absoluteFilePath(fileName(version, url)));
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
	{
		throw new MMCError(tr("Error: Trying to save %1: %2").arg(file.fileName(), file.errorString()));
		return;
	}
	file.write(data);
	file.close();

	MMC->quickmodslist()->markModAsExists(version->mod, version, file.fileName());
}
