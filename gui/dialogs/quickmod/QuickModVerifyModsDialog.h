#pragma once

#include <QDialog>

#include "logic/quickmod/QuickModMetadata.h"

namespace Ui
{
class QuickModVerifyModsDialog;
}

class QuickModVerifyModsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModVerifyModsDialog(QList<QuickModMetadataPtr> mods, QWidget *parent = 0);
	~QuickModVerifyModsDialog();

private:
	Ui::QuickModVerifyModsDialog *ui;
};
