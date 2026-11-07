#pragma once

#include <QObject>

class QWidget;

namespace TitleBar
{
enum class Style
{
    Default,    // the system's
    Brightness, // dark or light like the window's palette, the system's colours (and accent)
    Palette     // the window palette's colours: caption = Window, text = WindowText
};

/// Set up the native title bar (and border) of a top-level window, as far as the system can:
/// Windows 10 1809+ dark or light, Windows 11 any colour; macOS 10.14+ dark or light, the
/// palette's colour through a transparent title bar. Elsewhere it's the window manager's.
void apply(QWidget *window, Style style);

enum class Appearance
{
    System,
    Bright,
    Dark
};

/// The whole application's appearance, where the system has one: macOS (NSApp.appearance),
/// which native widgets, their palette and title bars follow. Elsewhere nothing: false.
bool setApplicationAppearance(Appearance appearance);
}

/// Sets up each top-level window's title bar when it gets its native window, when it is
/// shown and when its palette changes. Installed on the application, so it sees every
/// window: ours, QMessageBox, Qt's own QFileDialog (the native one isn't a Qt window).
class TitleBarWatcher : public QObject
{
public:
    using QObject::QObject;
    bool eventFilter(QObject *watched, QEvent *event) override;

    /// The style for all windows from now on, applied to the open ones at once: a theme
    /// change doesn't always change the palette, so PaletteChange alone isn't enough
    static void setStyle(TitleBar::Style style);

private:
    static TitleBar::Style s_style;
};
