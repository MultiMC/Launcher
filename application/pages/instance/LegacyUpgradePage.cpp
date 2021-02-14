#include "LegacyUpgradePage.h"
#include "ui_LegacyUpgradePage.h"

#include "InstanceList.h"
#include "minecraft/legacy/LegacyInstance.h"
#include "minecraft/legacy/LegacyUpgradeTask.h"
#include "Launcher.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ProgressDialog.h"

LegacyUpgradePage::LegacyUpgradePage(InstancePtr inst, QWidget *parent)
    : QWidget(parent), ui(new Ui::LegacyUpgradePage), m_inst(inst)
{
    ui->setupUi(this);
}

LegacyUpgradePage::~LegacyUpgradePage()
{
    delete ui;
}

void LegacyUpgradePage::runModalTask(Task *task)
{
    connect(task, &Task::failed, [this](QString reason)
        {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Warning)->show();
        });
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    if(loadDialog.execWithTask(task) == QDialog::Accepted)
    {
        m_container->requestClose();
    }
}

void LegacyUpgradePage::on_upgradeButton_clicked()
{
    QString newName = tr("%1 (Migrated)").arg(m_inst->name());
    auto upgradeTask = new LegacyUpgradeTask(m_inst);
    upgradeTask->setName(newName);
    upgradeTask->setGroup(LauncherPtr->instances()->getInstanceGroup(m_inst->id()));
    upgradeTask->setIcon(m_inst->iconKey());
    unique_qobject_ptr<Task> task(LauncherPtr->instances()->wrapInstanceTask(upgradeTask));
    runModalTask(task.get());
}

bool LegacyUpgradePage::shouldDisplay() const
{
    return !m_inst->isRunning();
}
