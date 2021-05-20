#pragma once

#include <QDialog>
#include "FtbFlaggedModsListModel.h"

namespace Ui
{
class FtbFlaggedModsDialog;
}

class FtbFlaggedModsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FtbFlaggedModsDialog(QWidget *parent, QVector<ModpacksCH::FlaggedMod> mods);
    ~FtbFlaggedModsDialog() override;

private:
    Ui::FtbFlaggedModsDialog *ui;

    FlaggedModsListModel *listModel;

};
