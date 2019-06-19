#include "CheckableInputDialog.h"

#include <QPushButton>
#include <QDebug>

#include "ui_CheckableInputDialog.h"

CheckableInputDialog::CheckableInputDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CheckableInputDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok), &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::StandardButton::Cancel), &QPushButton::clicked, this,
            &QDialog::reject);
}

CheckableInputDialog::~CheckableInputDialog()
{
    delete ui;
}

void CheckableInputDialog::setText(QString text)
{
    ui->label->setText(text);
}

void CheckableInputDialog::setExtraText(QString text)
{
    ui->extraText->setText(text);
}


void CheckableInputDialog::setCheckboxText(QString checkboxText)
{
    ui->checkBox->setText(checkboxText);
}

bool CheckableInputDialog::checkboxChecked()
{
    return ui->checkBox->checkState();
}

QString CheckableInputDialog::getInput()
{
    return ui->lineEdit->text();
}