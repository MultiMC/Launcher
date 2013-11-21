#ifndef CHOOSEQUICKMODVERSIONDIALOGKMODVERSIONDIALOG_H
#define CHOOSEQUICKMODVERSIONDIALOGKMODVERSIONDIALOG_H

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

	void setMod(const QuickMod* mod, const BaseInstance *instance);

	int version() const;

private:
	Ui::ChooseQuickModVersionDialog *ui;
};

#endif // CHOOSEQUICKMODVERSIONDIALOGKMODVERSIONDIALOG_H
