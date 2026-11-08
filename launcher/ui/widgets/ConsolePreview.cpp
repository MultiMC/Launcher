#include "ConsolePreview.h"

#include <QEvent>

#include "MessageLevel.h"
#include "ui/ColorCache.h"

ConsolePreview::ConsolePreview(QWidget *parent) : QTextEdit(parent)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void ConsolePreview::setConsoleFont(const QString &family, int size)
{
    m_family = family;
    m_size = size;
    refresh();
}

void ConsolePreview::setLogColors(const QMap<QString, QColor> &colors)
{
    m_logColors = colors;
    refresh();
}

void ConsolePreview::refresh()
{
    const QColor front = palette().color(foregroundRole());
    const QColor back = palette().color(backgroundRole());
    LogColorCache colors(front, back, m_logColors);
    const QFont font(m_family, m_size);
    clear();
    auto cursor = textCursor();
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

void ConsolePreview::changeEvent(QEvent *event)
{
    // the lines are in the palette's colours, and translated
    if(event->type() == QEvent::PaletteChange || event->type() == QEvent::LanguageChange)
    {
        refresh();
    }
    QTextEdit::changeEvent(event);
}
