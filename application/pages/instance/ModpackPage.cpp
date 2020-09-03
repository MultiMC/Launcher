#include "ModpackPage.h"
#include "ui_ModpackPage.h"

#include <QFileDialog>
#include <QDialog>
#include <QMessageBox>

#include "dialogs/VersionSelectDialog.h"
#include "JavaCommon.h"
#include "MultiMC.h"

#include <FileSystem.h>
#include "minecraft/ModpackInfo.h"


ModpackPage::ModpackPage(BaseInstance *inst, QWidget *parent)
    : QWidget(parent), ui(new Ui::ModpackPage), m_instance(inst)
{
    ui->setupUi(this);
    connect(ui->activateUIButton, &QCommandLinkButton::clicked, this, &ModpackPage::activateUIClicked);

    connect(ui->modpackCheck, &QCheckBox::stateChanged, this, &ModpackPage::updateState);
    connect(ui->javaArgumentsGroupBox, &QGroupBox::toggled, this, &ModpackPage::updateState);
    connect(ui->repositoryCheck, &QCheckBox::stateChanged, this, &ModpackPage::updateState);
    connect(ui->coordinateCheck, &QCheckBox::stateChanged, this, &ModpackPage::updateState);
    connect(ui->maxMemCheck, &QCheckBox::stateChanged, this, &ModpackPage::updateState);
    connect(ui->minMemCheck, &QCheckBox::stateChanged, this, &ModpackPage::updateState);
    connect(ui->optMemCheck, &QCheckBox::stateChanged, this, &ModpackPage::updateState);

    loadSettings();
    updateState();
}

void ModpackPage::updateState()
{
    bool enabled = uiActivated && ui->modpackCheck->isChecked();
    ui->modpackCheck->setEnabled(uiActivated);
    ui->platformComboBox->setEnabled(enabled);

    ui->javaArgumentsGroupBox->setEnabled(enabled);
    ui->jvmArgsTextBox->setEnabled(enabled && ui->javaArgumentsGroupBox->isChecked());

    ui->repositoryCheck->setEnabled(enabled);
    ui->repositoryEdit->setEnabled(enabled && ui->repositoryCheck->isChecked());

    ui->coordinateCheck->setEnabled(enabled);
    ui->coordinateEdit->setEnabled(enabled && ui->coordinateCheck->isChecked());

    ui->maxMemCheck->setEnabled(enabled);
    ui->maxMemSpinBox->setEnabled(enabled && ui->maxMemCheck->isChecked());

    ui->minMemCheck->setEnabled(enabled);
    ui->minMemSpinBox->setEnabled(enabled && ui->minMemCheck->isChecked());

    ui->optMemCheck->setEnabled(enabled);
    ui->optMemSpinBox->setEnabled(enabled && ui->optMemCheck->isChecked());
}

bool ModpackPage::shouldDisplay() const
{
    return !m_instance->isRunning();
}

ModpackPage::~ModpackPage()
{
    delete ui;
}

void ModpackPage::activateUIClicked(bool)
{
    uiActivated = true;
    ui->activateUIButton->setHidden(true);
    updateState();
}

bool ModpackPage::apply()
{
    applySettings();
    return true;
}

void ModpackPage::applySettings()
{
    PackProfileModpackInfo out;
    out.hasValue = ui->modpackCheck->isChecked();
    out.platform = ui->platformComboBox->currentText();
    if(ui->javaArgumentsGroupBox->isChecked()) {
        out.recommendedArgs = ui->jvmArgsTextBox->toPlainText();
    }
    if(ui->repositoryCheck->isChecked()) {
        out.repository = ui->repositoryEdit->text();
    }
    if(ui->coordinateCheck->isChecked()) {
        out.coordinate = ui->coordinateEdit->text();
    }
    if(ui->maxMemCheck->isChecked()) {
        out.maxHeap = ui->maxMemSpinBox->value();
    }
    if(ui->minMemCheck->isChecked()) {
        out.minHeap = ui->minMemSpinBox->value();
    }
    if(ui->optMemCheck->isChecked()) {
        out.optimalHeap = ui->optMemSpinBox->value();
    }
}

void ModpackPage::loadSettings()
{
    PackProfileModpackInfo in;

    ui->modpackCheck->setChecked(in.hasValue);
    // FIXME: validate?
    ui->platformComboBox->setCurrentText(in.platform);

    ui->javaArgumentsGroupBox->setChecked(!in.recommendedArgs.isNull());
    ui->jvmArgsTextBox->setPlainText(in.recommendedArgs);

    ui->repositoryCheck->setChecked(!in.repository.isNull());
    ui->repositoryEdit->setText(in.repository);

    ui->coordinateCheck->setChecked(!in.coordinate.isNull());
    ui->coordinateEdit->setText(in.coordinate);

    ui->maxMemCheck->setChecked(in.maxHeap);
    ui->maxMemSpinBox->setValue(in.maxHeap.get());

    ui->minMemCheck->setChecked(in.minHeap);
    ui->minMemSpinBox->setValue(in.minHeap.get());

    ui->optMemCheck->setChecked(in.optimalHeap);
    ui->optMemSpinBox->setValue(in.optimalHeap.get());
}
