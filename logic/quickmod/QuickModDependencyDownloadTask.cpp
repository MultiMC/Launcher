#include "QuickModDependencyDownloadTask.h"

#include "QuickMod.h"
#include "QuickModsList.h"
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

	connect(MMC->quickmodslist().get(), SIGNAL(modAdded(QuickMod *)), this,
			SLOT(modAdded(QuickMod *)));
	// TODO we cannot know if this is about us
	connect(MMC->quickmodslist().get(), &QuickModsList::error, [this](const QString &msg)
	{ emitFailed(msg); });

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
	double percentage = (double)current / (double)max;
	m_lastSetPercentage = qMax(m_lastSetPercentage, qCeil(percentage));
	setProgress(m_lastSetPercentage);
}

void QuickModDependencyDownloadTask::requestDependenciesOf(const QuickMod *mod)
{
	for (auto it = mod->references().begin(); it != mod->references().end(); ++it)
	{
		if (MMC->quickmodslist()->mod(it.key()))
		{
			continue;
		}
		if (!m_requestedMods.contains(it.key()))
		{
			MMC->quickmodslist()->registerMod(it.value());
			m_pendingMods.append(it.key());
			m_requestedMods.append(it.key());
		}
	}
	updateProgress();
}
