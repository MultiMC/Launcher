#include "SystemAppearance.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QTextStream>

#include "BrightTheme.h"
#include "DarkTheme.h"

// The XDG desktop portal's appearance settings: one interface for KDE, GNOME and their forks
// (whichever portal backend the desktop runs), with change notifications.
// https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html
namespace {
const char *PORTAL_SERVICE = "org.freedesktop.portal.Desktop";
const char *PORTAL_PATH = "/org/freedesktop/portal/desktop";
const char *SETTINGS_INTERFACE = "org.freedesktop.portal.Settings";
const char *APPEARANCE = "org.freedesktop.appearance";
// Don't hang the start when the portal is slow to come up
const int TIMEOUT_MS = 2000;

enum class Mode
{
    Unknown, // no portal, or it has no colour scheme
    Bright,
    Dark
};

// Settings.Read (version 1 of the interface, so older portals work too) wraps the value in a
// variant inside a variant
QVariant readSetting(const QString &key)
{
    auto message = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, SETTINGS_INTERFACE, "Read");
    message << QString(APPEARANCE) << key;
    auto reply = QDBusConnection::sessionBus().call(message, QDBus::Block, TIMEOUT_MS);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
    {
        qDebug() << "Portal setting" << key << "unavailable:" << reply.errorMessage();
        return QVariant();
    }
    QVariant value = reply.arguments().first();
    while (value.canConvert<QDBusVariant>())
        value = value.value<QDBusVariant>().variant();
    return value;
}

// color-scheme: 0 no preference (light, by GNOME's convention), 1 prefer dark, 2 prefer light
Mode readMode()
{
    QVariant value = readSetting("color-scheme");
    if (!value.isValid())
        return Mode::Unknown;
    return value.toUInt() == 1 ? Mode::Dark : Mode::Bright;
}

// accent-color: (r, g, b) as doubles in [0, 1]; anything outside means not set
SystemAppearance::Accent readAccent()
{
    QVariant value = readSetting("accent-color");
    if (!value.canConvert<QDBusArgument>())
        return SystemAppearance::Accent();
    double r = -1, g = -1, b = -1;
    const QDBusArgument argument = value.value<QDBusArgument>();
    argument.beginStructure();
    argument >> r >> g >> b;
    argument.endStructure();
    auto inRange = [](double c) { return c >= 0.0 && c <= 1.0; };
    if (!inRange(r) || !inRange(g) || !inRange(b))
        return SystemAppearance::Accent();
    return SystemAppearance::Accent::fromColor(QColor::fromRgbF(r, g, b));
}
}

// KDE's colour scheme, as KDE applications read it: kdeglobals in the user's config folder,
// [Colors:<set>] groups of "r,g,b" colours, written whenever the scheme or the accent changes.
// Only on KDE-like desktops (a leftover kdeglobals elsewhere doesn't count).
bool onKde()
{
    return qgetenv("KDE_FULL_SESSION") == "true" || QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")).contains("KDE", Qt::CaseInsensitive);
}

QString kdeGlobalsPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).absoluteFilePath("kdeglobals");
}

// The colours of the [Colors:*] groups, by group and key: a plain reading of the INI file
// (QSettings would take the commas for lists and the colons for something else)
QMap<QString, QMap<QString, QColor>> readKdeColors(const QString &path)
{
    QMap<QString, QMap<QString, QColor>> out;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    QTextStream in(&file);
    QString group;
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.startsWith('['))
        {
            group = line.mid(1, line.indexOf(']') - 1);
            continue;
        }
        const int equals = line.indexOf('=');
        if (!group.startsWith("Colors:") || equals < 0)
            continue;
        const QStringList rgb = line.mid(equals + 1).split(',');
        if (rgb.size() < 3)
            continue;
        QColor color(rgb[0].trimmed().toInt(), rgb[1].trimmed().toInt(), rgb[2].trimmed().toInt());
        if (color.isValid())
            out[group.mid(7)][line.left(equals).trimmed()] = color;
    }
    return out;
}

// The scheme as a palette, every role set: what it leaves out from our Bright or Dark,
// whichever is closer to its window colour. None (no roles set) without a scheme.
QPalette kdePalette(const QMap<QString, QMap<QString, QColor>> &scheme)
{
    QPalette none;
    none.resolve(0);
    const QColor window = scheme.value("Window").value("BackgroundNormal");
    if (!window.isValid())
        return none;
    const QPalette base = Colors::isDark(window) ? DarkTheme::colors() : BrightTheme::colors();
    auto color = [&](const char *set, const char *key, QPalette::ColorRole fallback) {
        const QColor c = scheme.value(set).value(key);
        return c.isValid() ? c : base.color(fallback);
    };
    const QColor button = color("Button", "BackgroundNormal", QPalette::Button);
    // QPalette(button, window) derives the shading roles (Light, Mid, Dark, Shadow)
    QPalette p(button, window);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, color("Window", "ForegroundNormal", QPalette::WindowText));
    p.setColor(QPalette::Base, color("View", "BackgroundNormal", QPalette::Base));
    p.setColor(QPalette::AlternateBase, color("View", "BackgroundAlternate", QPalette::AlternateBase));
    p.setColor(QPalette::Text, color("View", "ForegroundNormal", QPalette::Text));
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, color("Button", "ForegroundNormal", QPalette::ButtonText));
    p.setColor(QPalette::Highlight, color("Selection", "BackgroundNormal", QPalette::Highlight));
    p.setColor(QPalette::HighlightedText, color("Selection", "ForegroundNormal", QPalette::HighlightedText));
    p.setColor(QPalette::ToolTipBase, color("Tooltip", "BackgroundNormal", QPalette::ToolTipBase));
    p.setColor(QPalette::ToolTipText, color("Tooltip", "ForegroundNormal", QPalette::ToolTipText));
    p.setColor(QPalette::Link, color("View", "ForegroundLink", QPalette::Link));
    p.setColor(QPalette::LinkVisited, color("View", "ForegroundVisited", QPalette::LinkVisited));
    p.setColor(QPalette::BrightText, color("View", "ForegroundNegative", QPalette::BrightText));
    return p;
}

QPalette readPalette()
{
    if (!onKde())
    {
        QPalette none;
        none.resolve(0);
        return none;
    }
    return kdePalette(readKdeColors(kdeGlobalsPath()));
}

class SystemAppearance::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(SystemAppearance *q) : q(q), mode(readMode()), accent(readAccent()), palette(readPalette())
    {
        // KDE writes kdeglobals anew (a new file in its place), which a watch on the file
        // alone loses: the folder is watched too, and the file again after each change
        if (onKde())
        {
            watcher.addPath(QFileInfo(kdeGlobalsPath()).absolutePath());
            watcher.addPath(kdeGlobalsPath());
            connect(&watcher, &QFileSystemWatcher::fileChanged, this, &Private::schemeMaybeChanged);
            connect(&watcher, &QFileSystemWatcher::directoryChanged, this, &Private::schemeMaybeChanged);
            qDebug() << "KDE colour scheme:" << (palette.resolve() ? "read from" : "not in") << kdeGlobalsPath();
        }
        qDebug().noquote() << "Portal: colour scheme"
                           << (mode == Mode::Unknown ? "unknown" : (mode == Mode::Dark ? "dark" : "bright"))
                           << "accent" << (accent.isValid() ? accent.color.name() : QString("none"));
        QDBusConnection::sessionBus().connect(PORTAL_SERVICE, PORTAL_PATH, SETTINGS_INTERFACE, "SettingChanged", this,
                                              SLOT(settingChanged(QString, QString, QDBusVariant)));
    }

    SystemAppearance *q;
    Mode mode;
    Accent accent;
    QPalette palette;
    QFileSystemWatcher watcher;

private slots:
    void schemeMaybeChanged()
    {
        if (!watcher.files().contains(kdeGlobalsPath()) && QFileInfo::exists(kdeGlobalsPath()))
            watcher.addPath(kdeGlobalsPath());
        const QPalette now = readPalette();
        if (now == palette && now.resolve() == palette.resolve())
            return;
        palette = now;
        emit q->changed();
    }

    void settingChanged(const QString &ns, const QString &key, const QDBusVariant &)
    {
        if (ns != APPEARANCE || (key != "color-scheme" && key != "accent-color"))
            return;
        Mode nowMode = readMode();
        Accent nowAccent = readAccent();
        if (nowMode != mode || nowAccent != accent)
        {
            mode = nowMode;
            accent = nowAccent;
            emit q->changed();
        }
    }
};

SystemAppearance::SystemAppearance(QObject *parent) : QObject(parent), d(new Private(this))
{
}

SystemAppearance::~SystemAppearance()
{
    delete d;
}

bool SystemAppearance::canFollow() const
{
    return d->mode != Mode::Unknown;
}

bool SystemAppearance::isDark() const
{
    return d->mode == Mode::Dark;
}

SystemAppearance::Accent SystemAppearance::accent() const
{
    return d->accent;
}

// The portal says dark or light, not which colours. KDE's colour scheme has them: the System
// theme takes them with Fusion too, as the native style (Breeze) does. Elsewhere none, and the
// System theme uses our Bright or Dark.
QPalette SystemAppearance::palette() const
{
    return d->palette;
}

#include "SystemAppearance_linux.moc"
