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
    loadSettings();
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
    // FIXME: add a layer of indirection here
    if(out.hasValue) {
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
        if(ui->mimMemCheck->isChecked()) {
            out.minHeap = ui->minMemSpinBox->value();
        }
        if(ui->optMemCheck->isChecked()) {
            out.optimalHeap = ui->optMemSpinBox->value();
        }
    }

/*
struct PackProfileModpackInfo
{
    operator bool() const {
        return hasValue;
    }
    bool hasValue = false;

    QString platform;
    QString repository;
    QString coordinate;

    MaybeInt minHeap;
    MaybeInt optimalHeap;
    MaybeInt maxHeap;
    QString recommendedArgs;
};
*/
}

void ModpackPage::loadSettings()
{
    PackProfileModpackInfo in;

    // Console
    ui->consoleSettingsBox->setChecked(m_settings->get("OverrideConsole").toBool());
    ui->showConsoleCheck->setChecked(m_settings->get("ShowConsole").toBool());
    ui->autoCloseConsoleCheck->setChecked(m_settings->get("AutoCloseConsole").toBool());
    ui->showConsoleErrorCheck->setChecked(m_settings->get("ShowConsoleOnError").toBool());

    // Window Size
    ui->windowSizeGroupBox->setChecked(m_settings->get("OverrideWindow").toBool());
    ui->maximizedCheckBox->setChecked(m_settings->get("LaunchMaximized").toBool());
    ui->windowWidthSpinBox->setValue(m_settings->get("MinecraftWinWidth").toInt());
    ui->windowHeightSpinBox->setValue(m_settings->get("MinecraftWinHeight").toInt());

    // Memory
    ui->memoryGroupBox->setChecked(m_settings->get("OverrideMemory").toBool());
    int min = m_settings->get("MinMemAlloc").toInt();
    int max = m_settings->get("MaxMemAlloc").toInt();
    if(min < max)
    {
        ui->minMemSpinBox->setValue(min);
        ui->maxMemSpinBox->setValue(max);
    }
    else
    {
        ui->minMemSpinBox->setValue(max);
        ui->maxMemSpinBox->setValue(min);
    }
    ui->permGenSpinBox->setValue(m_settings->get("PermGen").toInt());
    bool permGenVisible = m_settings->get("PermGenVisible").toBool();
    ui->permGenSpinBox->setVisible(permGenVisible);
    ui->labelPermGen->setVisible(permGenVisible);
    ui->labelPermgenNote->setVisible(permGenVisible);


    // Java Settings
    bool overrideJava = m_settings->get("OverrideJava").toBool();
    bool overrideLocation = m_settings->get("OverrideJavaLocation").toBool() || overrideJava;
    bool overrideArgs = m_settings->get("OverrideJavaArgs").toBool() || overrideJava;

    ui->javaSettingsGroupBox->setChecked(overrideLocation);
    ui->javaPathTextBox->setText(m_settings->get("JavaPath").toString());

    ui->javaArgumentsGroupBox->setChecked(overrideArgs);
    ui->jvmArgsTextBox->setPlainText(m_settings->get("JvmArgs").toString());

    // Custom commands
    ui->customCommands->initialize(
        true,
        m_settings->get("OverrideCommands").toBool(),
        m_settings->get("PreLaunchCommand").toString(),
        m_settings->get("WrapperCommand").toString(),
        m_settings->get("PostExitCommand").toString()
    );
}
