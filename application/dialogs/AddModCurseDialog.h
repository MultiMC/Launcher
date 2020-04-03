#ifndef ADDMODCURSE_H
#define ADDMODCURSE_H

#include <QDialog>
#include <QString>
#include <QEvent>
#include <QKeyEvent>

#include "pages/BasePage.h"
#include <MultiMC.h>
#include "tasks/Task.h"
#include "pages/modplatform/twitch/TwitchModel.h"

namespace Ui {
class AddModCurseDialog;
}

class AddModCurseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddModCurseDialog(QWidget *parent = (QWidget*)nullptr);
    ~AddModCurseDialog();

    bool eventFilter(QObject * watched, QEvent * event) override;

    Twitch::Addon getResult() const
    {
        return current;
    }

private:
    void suggestCurrent();

private slots:
    virtual void accept() override;
    virtual void reject() override;
    void triggerSearch();
    void onSelectionChanged(QModelIndex first, QModelIndex second);

private:
    Twitch::ListModel* model = nullptr;
    Twitch::Addon current;
    Ui::AddModCurseDialog *ui;
};

#endif // ADDMODCURSE_H
