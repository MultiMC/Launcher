#pragma once

#include <QDialog>

namespace Ui {
class ChooseQuickModVersionDialog;
}

class QuickMod;
class BaseInstance;

class ChooseQuickModVersionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChooseQuickModVersionDialog(QWidget *parent = 0);
	~ChooseQuickModVersionDialog();

	int exec();

	void setCanCancel(bool canCancel);
	void setMod(const QuickMod* mod, const BaseInstance *instance, const QString &versionFilter);

	int version() const;

private:
	friend class ChooseQuickModVersionDialogTest;
	Ui::ChooseQuickModVersionDialog *ui;

	static bool versionIsInFilter(const QString &version, const QString &filter);
};
