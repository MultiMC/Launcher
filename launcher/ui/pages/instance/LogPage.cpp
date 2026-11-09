#include "LogPage.h"
#include "ui_LogPage.h"

#include "Application.h"

#include <QCheckBox>
#include <QIcon>
#include <QScrollBar>
#include <QShortcut>

#include "launch/LaunchTask.h"
#include "settings/Setting.h"

#include "ui/GuiUtil.h"
#include "ui/ColorCache.h"
#include "ui/themes/ThemeManager.h"
#include "ui/dialogs/CustomMessageBox.h"

#include <BuildConfig.h>

class LogFormatProxyModel : public QIdentityProxyModel
{
public:
    LogFormatProxyModel(QObject* parent = nullptr) : QIdentityProxyModel(parent)
    {
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        switch(role)
        {
            case Qt::FontRole:
                return m_font;
            case Qt::TextColorRole:
            {
                MessageLevel::Enum level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
                return m_colors->getFront(level);
            }
            case Qt::BackgroundRole:
            {
                MessageLevel::Enum level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
                return m_colors->getBack(level);
            }
            default:
                return QIdentityProxyModel::data(index, role);
            }
    }

    void setFont(QFont font)
    {
        m_font = font;
    }

    // the view shows everything again in the new colours
    void setColors(LogColorCache* colors)
    {
        beginResetModel();
        m_colors.reset(colors);
        endResetModel();
    }

    QModelIndex find(const QModelIndex &start, const QString &value, bool reverse) const
    {
        QModelIndex parentIndex = parent(start);
        auto compare = [&](int r) -> QModelIndex
        {
            QModelIndex idx = index(r, start.column(), parentIndex);
            if (!idx.isValid() || idx == start)
            {
                return QModelIndex();
            }
            QVariant v = data(idx, Qt::DisplayRole);
            QString t = v.toString();
            if (t.contains(value, Qt::CaseInsensitive))
                return idx;
            return QModelIndex();
        };
        if(reverse)
        {
            int from = start.row();
            int to = 0;

            for (int i = 0; i < 2; ++i)
            {
                for (int r = from; (r >= to); --r)
                {
                    auto idx = compare(r);
                    if(idx.isValid())
                        return idx;
                }
                // prepare for the next iteration
                from = rowCount() - 1;
                to = start.row();
            }
        }
        else
        {
            int from = start.row();
            int to = rowCount(parentIndex);

            for (int i = 0; i < 2; ++i)
            {
                for (int r = from; (r < to); ++r)
                {
                    auto idx = compare(r);
                    if(idx.isValid())
                        return idx;
                }
                // prepare for the next iteration
                from = 0;
                to = start.row();
            }
        }
        return QModelIndex();
    }
private:
    QFont m_font;
    std::unique_ptr<LogColorCache> m_colors;
};

void LogPage::updateColors()
{
    auto origForeground = ui->text->palette().color(ui->text->foregroundRole());
    auto origBackground = ui->text->palette().color(ui->text->backgroundRole());
    m_proxy->setColors(new LogColorCache(origForeground, origBackground, APPLICATION->themeManager()->logColors()));
}

LogPage::LogPage(InstancePtr instance, QWidget *parent)
    : QWidget(parent), ui(new Ui::LogPage), m_instance(instance)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();

    m_proxy = new LogFormatProxyModel(this);
    // set up text colors in the log proxy and adapt them to the current theme, again when it changes
    updateColors();
    connect(APPLICATION->themeManager(), &ThemeManager::applied, this, &LogPage::updateColors);

    // set up fonts in the log proxy
    {
        QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
        bool conversionOk = false;
        int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
        if(!conversionOk)
        {
            fontSize = 11;
        }
        m_proxy->setFont(QFont(fontFamily, fontSize));
    }

    ui->text->setModel(m_proxy);

    // set up instance and launch process recognition
    {
        auto launchTask = m_instance->getLaunchTask();
        if(launchTask)
        {
            setInstanceLaunchTaskChanged(launchTask, true);
        }
        connect(m_instance.get(), &BaseInstance::launchTaskChanged, this, &LogPage::onInstanceLaunchTaskChanged);
    }

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, SIGNAL(activated()), SLOT(findActivated()));
    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, SIGNAL(activated()), SLOT(findNextActivated()));
    connect(ui->searchBar, SIGNAL(returnPressed()), SLOT(on_findButton_clicked()));
    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, SIGNAL(activated()), SLOT(findPreviousActivated()));
}

LogPage::~LogPage()
{
    delete ui;
}

void LogPage::modelStateToUI()
{
    if(m_model->wrapLines())
    {
        ui->text->setWordWrap(true);
        ui->wrapCheckbox->setCheckState(Qt::Checked);
    }
    else
    {
        ui->text->setWordWrap(false);
        ui->wrapCheckbox->setCheckState(Qt::Unchecked);
    }
    if(m_model->suspended())
    {
        ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
    }
    else
    {
        ui->trackLogCheckbox->setCheckState(Qt::Checked);
    }
}

void LogPage::UIToModelState()
{
    if(!m_model)
    {
        return;
    }
    m_model->setLineWrap(ui->wrapCheckbox->checkState() == Qt::Checked);
    m_model->suspend(ui->trackLogCheckbox->checkState() != Qt::Checked);
}

void LogPage::setInstanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc, bool initial)
{
    m_process = proc;
    if(m_process)
    {
        m_model = proc->getLogModel();
        m_proxy->setSourceModel(m_model.get());
        if(initial)
        {
            modelStateToUI();
        }
        else
        {
            UIToModelState();
        }
    }
    else
    {
        m_proxy->setSourceModel(nullptr);
        m_model.reset();
    }
}

void LogPage::onInstanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc)
{
    setInstanceLaunchTaskChanged(proc, false);
}

bool LogPage::apply()
{
    return true;
}

bool LogPage::shouldDisplay() const
{
    return m_instance->isRunning() || m_proxy->rowCount() > 0;
}

void LogPage::on_btnPaste_clicked()
{
    if(!m_model)
        return;

    auto response = CustomMessageBox::selectable(
            this,
            tr("Log upload"),
            tr("Are you sure you want to upload this log file?"),
            QMessageBox::Warning,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
    )->exec();

    if (response != QMessageBox::Yes)
        return;

    //FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
    m_model->append(
        MessageLevel::Launcher,
        QString("%2: Log upload triggered at: %1").arg(
            QDateTime::currentDateTime().toString(Qt::RFC2822Date),
            BuildConfig.LAUNCHER_NAME
        )
    );
    auto url = GuiUtil::uploadPaste(m_model->toPlainText(), this);
    if(!url.isEmpty())
    {
        m_model->append(
            MessageLevel::Launcher,
            QString("%2: Log uploaded to: %1").arg(
                url,
                BuildConfig.LAUNCHER_NAME
            )
        );
    }
    else
    {
        m_model->append(
            MessageLevel::Error,
            QString("%1: Log upload failed!").arg(BuildConfig.LAUNCHER_NAME)
        );
    }
}

void LogPage::on_btnCopy_clicked()
{
    if(!m_model)
        return;
    m_model->append(MessageLevel::Launcher, QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
    GuiUtil::setClipboardText(m_model->toPlainText());
}

void LogPage::on_btnClear_clicked()
{
    if(!m_model)
        return;
    auto settings = APPLICATION->settings();
    if(settings->get("ConfirmLogClear").toBool())
    {
        QMessageBox question(QMessageBox::Question, tr("Clear"),
                             tr("Do you really want to clear the log? What it shows now can't be brought back."),
                             QMessageBox::Yes | QMessageBox::No, this);
        question.setDefaultButton(QMessageBox::No);
        auto dontAsk = new QCheckBox(tr("Don't ask again"));
        question.setCheckBox(dontAsk);
        if(question.exec() != QMessageBox::Yes)
            return;
        // only with a yes: a "no" doesn't turn the question off
        if(dontAsk->isChecked())
            settings->set("ConfirmLogClear", false);
    }
    m_model->clear();
    m_container->refreshContainer();
}

void LogPage::on_btnBottom_clicked()
{
    ui->text->scrollToBottom();
}

void LogPage::on_trackLogCheckbox_clicked(bool checked)
{
    if(!m_model)
        return;
    m_model->suspend(!checked);
}

void LogPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if(!m_model)
        return;
    m_model->setLineWrap(checked);
}

void LogPage::on_findButton_clicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    ui->text->findNext(ui->searchBar->text(), reverse);
}

void LogPage::findNextActivated()
{
    ui->text->findNext(ui->searchBar->text(), false);
}

void LogPage::findPreviousActivated()
{
    ui->text->findNext(ui->searchBar->text(), true);
}

void LogPage::findActivated()
{
    // focus the search bar if it doesn't have focus
    if (!ui->searchBar->hasFocus())
    {
        ui->searchBar->setFocus();
        ui->searchBar->selectAll();
    }
}
