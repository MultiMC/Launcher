#include "QuickModVerificationTask.h"

#include <QMessageBox>
#include <QPushButton>

#include "logic/net/ByteArrayDownload.h"
#include "logic/quickmod/QuickMod.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "MultiMC.h"

// TODO all of this might need some more work

QuickModVerificationTask::QuickModVerificationTask(const QList<QuickModVersionPtr> &modVersions, QObject *parent)
	: Task(parent), m_modVersions(modVersions)
{

}

void QuickModVerificationTask::executeTask()
{
	m_netJob = NetJobPtr(new NetJob(tr("QuickMod verification")));
	for (const QuickModVersionPtr modVersion : m_modVersions)
	{
		if (!modVersion->mod->verifyUrl().isValid())
		{
			continue;
		}
		m_netJob->addNetAction(ByteArrayDownload::make(modVersion->mod->verifyUrl()));
	}
	connect(m_netJob.get(), &NetJob::succeeded, this, &Task::successful);
	connect(m_netJob.get(), &NetJob::failed, [this](){ emitFailed(QString()); });
	connect(m_netJob.get(), &NetJob::progress, [this](const qint64 current, const qint64 total) { setProgress(current/total); });
	m_netJob->start();
}

void QuickModVerificationTask::downloadSucceeded(int index)
{
	const QuickModVersionPtr modVersion = m_modVersions.at(index);
	const QByteArray actual = std::dynamic_pointer_cast<ByteArrayDownload>(m_netJob->at(index))->m_data;
	const QByteArray expected = modVersion->mod->hash().toHex();
	if (actual == expected)
	{
		if (!MMC->quickmodslist()->isWebsiteTrusted(modVersion->mod->verifyUrl()))
		{
			QMessageBox box;
			box.setWindowTitle(tr("Trust website?"));
			box.setText(tr("The QuickMod %1 verifies against %2.\nTrust %2?")
							.arg(modVersion->mod->name(), modVersion->mod->verifyUrl().host()));
			box.setIcon(QMessageBox::Question);
			QPushButton *once = box.addButton(tr("Once"), QMessageBox::AcceptRole);
			QPushButton *always = box.addButton(tr("Always"), QMessageBox::AcceptRole);
			QPushButton *no = box.addButton(QMessageBox::No);
			box.setDefaultButton(always);
			box.exec();
			if (box.clickedButton() == no)
			{
				emitFailed(tr("%1 did not pass verification").arg(modVersion->mod->name()));
			}
			if (box.clickedButton() == always)
			{
				MMC->quickmodslist()->setWebsiteTrusted(modVersion->mod->verifyUrl(), true);
			}
		}
	}
	else
	{
		QMessageBox box;
		box.setWindowTitle(tr("Trust website?"));
		box.setText(tr("The QuickMod %1 does NOT verify against %2.")
						.arg(modVersion->mod->name(), modVersion->mod->verifyUrl().host()));
		box.setInformativeText(tr("You may add a one-time exception, but it's strictly "
								  "recommended that you don't."));
		box.setIcon(QMessageBox::Warning);
		QPushButton *exception =
			box.addButton(tr("Add one-time exception"), QMessageBox::AcceptRole);
		QPushButton *no = box.addButton(QMessageBox::No);
		box.setDefaultButton(no);
		box.exec();
		if (box.clickedButton() == no)
		{
			emitFailed(tr("%1 did not pass verification").arg(modVersion->mod->name()));
		}
	}
}

void QuickModVerificationTask::downloadFailure(int index)
{
	const QuickModVersionPtr modVersion = m_modVersions.at(index);
}
