#include "VanillaPage.h"
#include "ui_VanillaPage.h"

#include "MultiMC.h"

#include <meta/Index.h>
#include <meta/VersionList.h>
#include <dialogs/NewInstanceDialog.h>
#include <Filter.h>
#include <Env.h>
#include <InstanceCreationTask.h>
#include <QTabBar>

VanillaPage::VanillaPage(NewInstanceDialog *dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::VanillaPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    connect(ui->versionSelect, &VersionSelectWidget::selectedVersionChanged, this, &VanillaPage::setSelectedVersion);
}

void VanillaPage::openedImpl()
{
    if(!initialized)
    {
        auto vlist = ENV.metadataIndex()->get("net.minecraft");
        ui->versionSelect->initialize(vlist.get());
        initialized = true;
    }
    else
    {
        suggestCurrent();
    }
}

VanillaPage::~VanillaPage()
{
    delete ui;
}

bool VanillaPage::shouldDisplay() const
{
    return true;
}

BaseVersionPtr VanillaPage::selectedVersion() const
{
    return m_selectedVersion;
}

void VanillaPage::suggestCurrent()
{
    if(m_selectedVersion && isOpened)
    {
        dialog->setSuggestedPack(m_selectedVersion->descriptor(), new InstanceCreationTask(m_selectedVersion));
    }
}

void VanillaPage::setSelectedVersion(BaseVersionPtr version)
{
    m_selectedVersion = version;
    suggestCurrent();
}
