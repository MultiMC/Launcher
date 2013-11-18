#pragma once

#include <QDialog>
#include <QList>
#include "logic/QuickModFilesUpdater.h"

namespace Ui {
class DownloadProgressDialog;
}

class DownloadProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadProgressDialog(int modcount, QWidget *parent = 0);
    ~DownloadProgressDialog();
    QList<QuickMod*> mods();

public slots:
    void ModAdded(QuickMod *mod);

private:
    Ui::DownloadProgressDialog *ui;
    QList<QuickMod*> m_downloadedmods;
    int m_modcount;
};
