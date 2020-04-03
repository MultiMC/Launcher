#include "AddModCurseDialog.h"
#include "ui_AddModCurseDialog.h"

AddModCurseDialog::AddModCurseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddModCurseDialog)
{
    ui->setupUi(this);

    connect(ui->searchButton,
            &QPushButton::clicked,
            this,
            &AddModCurseDialog::triggerSearch);

    ui->searchEdit->installEventFilter(this);

    model = new Twitch::ListModel(this, Twitch::Mod);

    ui->modView->setModel(model);

    connect(ui->modView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &AddModCurseDialog::onSelectionChanged);

    ui->buttonBox->buttons()[0]->setEnabled(false);
}

bool AddModCurseDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AddModCurseDialog::triggerSearch()
{
    model->searchWithTerm(ui->searchEdit->text());
}

void AddModCurseDialog::accept()
{
    QDialog::accept();
} 

void AddModCurseDialog::reject()
{
    QDialog::reject();
}

void AddModCurseDialog::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    if(!first.isValid())
    {
        return;
    }
    current = model->data(first, Qt::UserRole).value<Twitch::Addon>();
    ui->buttonBox->buttons()[0]->setEnabled(true);
}

AddModCurseDialog::~AddModCurseDialog()
{
    delete ui;
}
