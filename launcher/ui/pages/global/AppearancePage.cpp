#include "AppearancePage.h"

#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "MessageLevel.h"
#include "settings/SettingsObject.h"
#include "ui/ColorCache.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/IconThemeSelectionWidget.h"
#include "ui/widgets/ThemeSelectionWidget.h"

AppearancePage::AppearancePage(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("appearancePage"));
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs);

    m_themeWidget = new ThemeSelectionWidget(m_tabs);
    m_tabs->addTab(m_themeWidget, QString());
    // at once, like in the setup wizard: the dialog has no Cancel to take it back
    connect(m_themeWidget, &ThemeSelectionWidget::choiceChanged, this, [](const ThemeManager::Choice &choice)
    {
        auto manager = APPLICATION->themeManager();
        manager->save(choice);
        manager->apply(choice);
    });

    m_iconsWidget = new IconThemeSelectionWidget(m_tabs);
    m_tabs->addTab(m_iconsWidget, QString());
    connect(m_iconsWidget, &IconThemeSelectionWidget::iconThemeChanged, this, [](const QString &id)
    {
        APPLICATION->settings()->set("IconTheme", id);
        APPLICATION->setIconTheme(id);
    });

    m_consoleTab = makeConsoleTab();
    m_tabs->addTab(m_consoleTab, QString());

    retranslate();
    loadSettings();
}

QWidget *AppearancePage::makeConsoleTab()
{
    auto tab = new QWidget(m_tabs);
    auto layout = new QVBoxLayout(tab);
    auto fontRow = new QHBoxLayout();
    m_fontLabel = new QLabel(tab);
    m_consoleFont = new QFontComboBox(tab);
    m_consoleFont->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_fontSize = new QSpinBox(tab);
    m_fontSize->setRange(5, 16);
    m_fontLabel->setBuddy(m_consoleFont);
    fontRow->addWidget(m_fontLabel);
    fontRow->addWidget(m_consoleFont, 1);
    fontRow->addWidget(m_fontSize);
    layout->addLayout(fontRow);
    m_consolePreview = new QTextEdit(tab);
    m_consolePreview->setReadOnly(true);
    m_consolePreview->setUndoRedoEnabled(false);
    m_consolePreview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_consolePreview, 1);

    connect(m_consoleFont, &QFontComboBox::currentFontChanged, this, &AppearancePage::consoleFontChanged);
    connect(m_fontSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &AppearancePage::consoleFontChanged);
    // the theme's log colours
    connect(APPLICATION->themeManager(), &ThemeManager::applied, this, &AppearancePage::updateConsolePreview);
    return tab;
}

void AppearancePage::consoleFontChanged()
{
    if(!m_loading)
    {
        auto s = APPLICATION->settings();
        s->set("ConsoleFont", m_consoleFont->currentFont().family());
        s->set("ConsoleFontSize", m_fontSize->value());
    }
    updateConsolePreview();
}

void AppearancePage::updateConsolePreview()
{
    const QColor front = m_consolePreview->palette().color(m_consolePreview->foregroundRole());
    const QColor back = m_consolePreview->palette().color(m_consolePreview->backgroundRole());
    LogColorCache colors(front, back, APPLICATION->themeManager()->logColors());
    const QFont font(m_consoleFont->currentFont().family(), m_fontSize->value());
    m_consolePreview->clear();
    auto cursor = m_consolePreview->textCursor();
    // a line for each kind of message, in the colours the console uses
    const QList<QPair<MessageLevel::Enum, QString>> lines = {
        {MessageLevel::Launcher, tr("Launcher: preparing the instance...")},
        {MessageLevel::Message, tr("[Test/INFO] A harmless message...")},
        {MessageLevel::Debug, tr("[Test/DEBUG] Details for the curious.")},
        {MessageLevel::Warning, tr("[Something/WARN] A not so spooky warning.")},
        {MessageLevel::Error, tr("[Something/ERROR] A spooky error!")},
        {MessageLevel::Fatal, tr("[Something/FATAL] The game crashed.")},
    };
    for(auto &line : lines)
    {
        QTextCharFormat format;
        format.setFont(font);
        format.setForeground(colors.getFront(line.first));
        const QColor background = colors.getBack(line.first);
        if(background.alpha() > 0)
        {
            format.setBackground(background);
        }
        cursor.insertText(line.second, format);
        if(line.first != MessageLevel::Fatal)
        {
            cursor.insertBlock();
        }
    }
}

void AppearancePage::loadSettings()
{
    auto s = APPLICATION->settings();
    m_iconsWidget->setIconTheme(s->get("IconTheme").toString());
    m_themeWidget->setChoice(APPLICATION->themeManager()->savedChoice());

    m_loading = true;
    m_consoleFont->setCurrentFont(QFont(s->get("ConsoleFont").toString()));
    bool conversionOk = true;
    int fontSize = s->get("ConsoleFontSize").toInt(&conversionOk);
    m_fontSize->setValue(conversionOk ? fontSize : 11);
    m_loading = false;
    updateConsolePreview();
}

void AppearancePage::retranslate()
{
    m_tabs->setTabText(m_tabs->indexOf(m_themeWidget), tr("Theme"));
    m_tabs->setTabText(m_tabs->indexOf(m_iconsWidget), tr("Icons"));
    m_tabs->setTabText(m_tabs->indexOf(m_consoleTab), tr("Console"));
    m_fontLabel->setText(tr("&Font:"));
    updateConsolePreview();
}

void AppearancePage::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
