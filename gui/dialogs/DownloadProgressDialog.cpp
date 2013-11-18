#include "DownloadProgressDialog.h"
#include "ui_DownloadProgressDialog.h"

DownloadProgressDialog::DownloadProgressDialog(int modcount, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DownloadProgressDialog),
    m_modcount(modcount)
{
    ui->setupUi(this);
    ui->progressBar->setMaximum(m_modcount);
}

DownloadProgressDialog::~DownloadProgressDialog()
{
    delete ui;
}

void DownloadProgressDialog::ModAdded(QuickMod *mod)
{
    m_downloadedmods.append(mod);
    if(ui->progressBar->value() +1 == m_modcount)
    {
        accept();
    }
    ui->progressBar->setValue(ui->progressBar->value() + 1);
}

QList<QuickMod*> DownloadProgressDialog::mods()
{
    return m_downloadedmods;
}
