#include <modplatform/atlauncher/ATLPackInstallTask.h>
#include "AtlPage.h"
#include "ui_AtlPage.h"

#include "dialogs/NewInstanceDialog.h"

AtlPage::AtlPage(NewInstanceDialog* dialog, QWidget *parent)
        : QWidget(parent), ui(new Ui::AtlPage), dialog(dialog)
{
    ui->setupUi(this);
    model = new Atl::ListModel(this);
    ui->packView->setModel(model);

    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &AtlPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &AtlPage::onVersionSelectionChanged);
}

AtlPage::~AtlPage()
{
    delete ui;
}

bool AtlPage::shouldDisplay() const
{
    return true;
}

void AtlPage::openedImpl()
{
    model->request();
}

void AtlPage::suggestCurrent()
{
    if(isOpened) {
        dialog->setSuggestedPack(selected.name, new ATLauncher::PackInstallTask(selected.safeName, selectedVersion));
    }
}

void AtlPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }

    selected = model->data(first, Qt::UserRole).value<ATLauncher::IndexedPack>();

    for(const auto& version : selected.versions) {
        ui->versionSelectionBox->addItem(version.version);
    }

    suggestCurrent();
}

void AtlPage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }

    selectedVersion = data;
    suggestCurrent();
}
