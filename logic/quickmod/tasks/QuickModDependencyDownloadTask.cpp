#include "QuickModDependencyDownloadTask.h"

#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModsList.h"
#include "MultiMC.h"

QuickModDependencyDownloadTask::QuickModDependencyDownloadTask(QList<QuickMod *> mods,
															   QObject *parent)
	: Task(parent), m_mods(mods)
{
}

void QuickModDependencyDownloadTask::executeTask()
{
	m_pendingMods.clear();
	m_lastSetPercentage = 0;

	setStatus(tr("Fetching QuickMods files..."));

	connect(MMC->quickmodslist().get(), &QuickModsList::modAdded,
			this, &QuickModDependencyDownloadTask::modAdded);
	// TODO we cannot know if this is about us
	connect(MMC->quickmodslist().get(), &QuickModsList::error, this, &QuickModDependencyDownloadTask::emitFailed);

	foreach(const QuickMod * mod, m_mods)
	{
		requestDependenciesOf(mod);
	}
	if (m_pendingMods.isEmpty())
	{
		emitSucceeded();
	}
	updateProgress();
}

void QuickModDependencyDownloadTask::modAdded(QuickMod *mod)
{
	if (m_pendingMods.contains(mod->uid()))
	{
		m_pendingMods.removeAll(mod->uid());
		m_mods.append(mod);
		requestDependenciesOf(mod);
	}
	updateProgress();
}

void QuickModDependencyDownloadTask::updateProgress()
{
	int max = m_requestedMods.size();
	int current = max - m_pendingMods.size();
	double percentage = (double)current * 100.0f / (double)max;
	m_lastSetPercentage = qMax(m_lastSetPercentage, qCeil(percentage));
	setProgress(m_lastSetPercentage);
	if (m_pendingMods.isEmpty())
	{
		emitSucceeded();
	}
}

void QuickModDependencyDownloadTask::requestDependenciesOf(const QuickMod *mod)
{
	auto references = mod->references();
	qDebug() << mod->uid() << references;
	for (auto it = references.begin(); it != references.end(); ++it)
	{
		const QString modUid = it.key();
		if (!m_requestedMods.contains(modUid))
		{
			MMC->quickmodslist()->registerMod(it.value());
			m_pendingMods.append(modUid);
			m_requestedMods.append(modUid);
		}
	}
}
