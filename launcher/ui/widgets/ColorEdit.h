#pragma once

#include <QColor>
#include <QWidget>

class QLineEdit;
class QToolButton;
class ColorSwatch;

/**
 * A colour to type or pick: a swatch of it, its hex code to edit (#rrggbb; a colour's name
 * works too), and a button for a picker. Typing shows each valid colour as it comes
 * (colorTried); Enter or leaving the field sets it (colorEdited), and an invalid one goes back
 * to the colour it was. The picker is the owner's to open (pickRequested): its button, or Enter.
 * Up and down go to the colours above and below. Unset, it shows a placeholder and the colour
 * that applies instead.
 */
class ColorEdit : public QWidget
{
    Q_OBJECT
public:
    explicit ColorEdit(QWidget *parent = nullptr);
    virtual ~ColorEdit() {};

    /// The colour; invalid: unset, the placeholder text shown and shown in the swatch
    void setColor(const QColor &color, const QColor &shown = QColor());
    QColor color() const;
    void setPlaceholderText(const QString &text);
    QLineEdit *lineEdit() const;
    /// Typing not set yet goes: the colour as it was (editingAbandoned). False: none.
    bool abandonEditing();

signals:
    /// A valid colour typed, not set yet
    void colorTried(const QColor &color);
    /// A colour typed and set, different from the one before
    void colorEdited(const QColor &color);
    /// The typing given up (an invalid colour, or the same): the colour as it was applies
    void editingAbandoned();
    void pickRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void finishEditing();
    /// Focus to the colour step rows below (or above, negative), among the ones beside it
    void moveTo(int step);
    void showSwatch();

private: /* data */
    ColorSwatch *m_swatch = nullptr;
    QLineEdit *m_edit = nullptr;
    QToolButton *m_pick = nullptr;
    QColor m_color;
    QColor m_shown;
};
