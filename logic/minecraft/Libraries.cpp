#include "Libraries.h"

#include <modutils.h>

#include "minecraft/VersionBuildError.h"
#include "tasks/Task.h"
#include "net/NetJob.h"

#include "Env.h"
#include "minecraft/Mod.h"
#include "Functional.h"

namespace Minecraft
{

int findLibraryByName(QList<LibraryPtr> haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (haystack.at(i)->name().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

QList<LibraryPtr> Libraries::getActiveLibs() const
{
	QList<LibraryPtr> out;
	for (const LibraryPtr &ptr : overwriteLibs)
	{
		if (ptr->isActive())
		{
			for (const LibraryPtr &other : out)
			{
				if (other->name() == ptr->name())
				{
					qWarning() << "Multiple libraries with name" << ptr->name()
							   << "in library list!";
					continue;
				}
			}
			out.append(ptr);
		}
	}
	return out;
}

Libraries::Libraries(const QList<LibraryPtr> &libraries)
	: DownloadableResource(Functional::map(&std::dynamic_pointer_cast<BaseDownload, Library>, libraries))
{
}

void Libraries::applyTo(const ResourcePtr &target) const
{
	std::shared_ptr<Libraries> other = std::dynamic_pointer_cast<Libraries>(target);
	if (shouldOverwriteLibs)
	{
		other->overwriteLibs = overwriteLibs;
	}
	for (auto addedLibrary : addLibs)
	{
		switch (addedLibrary->insertType)
		{
		case Library::Apply:
		{
			// qDebug() << "Applying lib " << lib->name;
			int index = findLibraryByName(overwriteLibs, addedLibrary->name());
			if (index >= 0)
			{
				addedLibrary->applyTo(overwriteLibs[index]);
			}
			else
			{
				qWarning() << "Couldn't find" << addedLibrary->name() << "(skipping)";
			}
			break;
		}
		case Library::Append:
		case Library::Prepend:
		{
			// find the library by name.
			const int index = findLibraryByName(other->overwriteLibs, addedLibrary->name());
			// library not found? just add it.
			if (index < 0)
			{
				if (addedLibrary->insertType == Library::Append)
				{
					other->overwriteLibs.append(addedLibrary);
				}
				else
				{
					other->overwriteLibs.prepend(addedLibrary);
				}
				break;
			}

			// otherwise apply differences, if allowed
			auto existingLibrary = other->overwriteLibs.at(index);
			const Util::Version addedVersion = addedLibrary->name().version();
			const Util::Version existingVersion = existingLibrary->name().version();
			// if the existing version is a hard dependency we can either use it or
			// fail, but we can't change it
			if (existingLibrary->dependType == Library::Hard)
			{
				// we need a higher version, or we're hard to and the versions aren't equal
				if (addedVersion > existingVersion ||
					(addedLibrary->dependType == Library::Hard &&
					 addedVersion != existingVersion))
				{
					throw VersionBuildError(
						QObject::tr("Error resolving library dependencies between %1 and %2.")
							.arg(existingLibrary->name(), addedLibrary->name()));
				}
				else
				{
					// the library is already existing, so we don't have to do anything
				}
			}
			else if (existingLibrary->dependType == Library::Soft)
			{
				// if we are higher it means we should update
				if (addedVersion > existingVersion)
				{
					other->overwriteLibs.replace(index, addedLibrary);
				}
				else
				{
					// our version is smaller than the existing version, but we require
					// it: fail
					if (addedLibrary->dependType == Library::Hard)
					{
						throw VersionBuildError(
							QObject::tr(
								"Error resolving library dependencies between %1 and %2.")
								.arg(existingLibrary->name(), addedLibrary->name()));
					}
				}
			}
			break;
		}
		case Library::Replace:
		{
			GradleSpecifier toReplace;
			if (addedLibrary->insertData.isEmpty())
			{
				toReplace = addedLibrary->name();
			}
			else
			{
				toReplace = addedLibrary->insertData;
			}
			// qDebug() << "Replacing lib " << toReplace << " with " << lib->name;
			int index = findLibraryByName(other->overwriteLibs, toReplace);
			if (index >= 0)
			{
				other->overwriteLibs.replace(index, addedLibrary);
			}
			else
			{
				qWarning() << "Couldn't find" << toReplace << "(skipping)";
			}
			break;
		}
		}
	}
	for (auto lib : removeLibs)
	{
		int index = findLibraryByName(other->overwriteLibs, lib);
		if (index >= 0)
		{
			// qDebug() << "Removing lib " << lib;
			other->overwriteLibs.removeAt(index);
		}
		else
		{
			qWarning() << "Couldn't find" << lib << "(skipping)";
		}
	}
}

class JarlibUpdate : public Task
{
	Q_OBJECT
public:
	explicit JarlibUpdate(const Libraries *libs, QObject *parent = nullptr)
		: Task(parent), m_libs(libs)
	{
	}
	void executeTask() override
	{
		jarlibStart();
	}

private slots:
	void jarlibStart()
	{
		setStatus(tr("Getting the library files from Mojang..."));
		qDebug() << "downloading libraries";

		auto job = new NetJob(tr("Libraries"));
		jarlibDownloadJob.reset(job);

		QList<LibraryPtr> brokenLocalLibs;

		for (auto lib : m_libs->getActiveLibs())
		{
			try
			{
				for (NetActionPtr ptr : lib->createNetActions())
				{
					job->addNetAction(ptr);
				}
			}
			catch (...)
			{
				brokenLocalLibs.append(lib);
			}
		}
		if (!brokenLocalLibs.empty())
		{
			jarlibDownloadJob.reset();
			QStringList failed;
			for (auto brokenLib : brokenLocalLibs)
			{
				failed.append(brokenLib->files());
			}
			QString failed_all = failed.join("\n");
			emitFailed(
				tr("Some libraries marked as 'local' are missing their jar "
				   "files:\n%1\n\nYou'll have to correct this problem manually. If this is "
				   "an externally tracked instance, make sure to run it at least once "
				   "outside of MultiMC.").arg(failed_all));
			return;
		}

		connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(jarlibFinished()));
		connect(jarlibDownloadJob.get(), SIGNAL(failed(QString)), SLOT(jarlibFailed(QString)));
		connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
				SIGNAL(progress(qint64, qint64)));

		jarlibDownloadJob->start();
	}
	void jarlibFinished()
	{
		emitSucceeded();
	}
	void jarlibFailed(QString error)
	{
		QStringList failed = jarlibDownloadJob->getFailedFiles();
		QString failed_all = failed.join("\n");
		emitFailed(tr("Failed to download the following files:\n%1\n\nPlease try again.")
					   .arg(failed_all));
	}

private:
	NetJobPtr jarlibDownloadJob;
	const Libraries *m_libs;
};

Task *Libraries::updateTask() const
{
	return new JarlibUpdate(this);
}

ResourcePtr LibrariesFactory::create(const int formatVersion, const QString &key, const QJsonValue &data) const
{
	QList<LibraryPtr> downloads;
	for (const QJsonObject &obj : Json::ensureIsArrayOf<QJsonObject>(data))
	{
		downloads.append(std::dynamic_pointer_cast<Library>(createDownload(obj)));
	}
	std::shared_ptr<Libraries> libs = std::make_shared<Libraries>(downloads);
	libs->addLibs = downloads;
	return libs;
}
DownloadPtr LibrariesFactory::createDownload(const QJsonObject &obj) const
{
	using namespace Json;
	LibraryPtr lib = std::make_shared<Library>(QUrl(), ensureInteger(obj, "size"), ensureByteArray(obj, "sha256"));
	lib->m_name = GradleSpecifier(ensureString(obj, "name"));
	lib->m_base_url = ensureUrl(obj, "mavenBaseUrl", QUrl());
	lib->m_absolute_url = ensureUrl(obj, "url", lib->url());
	return lib;
}

}

#include "Libraries.moc"
