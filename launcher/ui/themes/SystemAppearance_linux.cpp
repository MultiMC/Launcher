#include "SystemAppearance.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDebug>

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

class SystemAppearance::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(SystemAppearance *q) : q(q), mode(readMode()), accent(readAccent())
    {
        qDebug().noquote() << "Portal: colour scheme"
                           << (mode == Mode::Unknown ? "unknown" : (mode == Mode::Dark ? "dark" : "bright"))
                           << "accent" << (accent.isValid() ? accent.color.name() : QString("none"));
        QDBusConnection::sessionBus().connect(PORTAL_SERVICE, PORTAL_PATH, SETTINGS_INTERFACE, "SettingChanged", this,
                                              SLOT(settingChanged(QString, QString, QDBusVariant)));
    }

    SystemAppearance *q;
    Mode mode;
    Accent accent;

private slots:
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

// The portal says dark or light, not which colours: the System theme uses our Bright or Dark.
// (KDE-like desktops give their real colours to the native style, see ThemeManager.)
QPalette SystemAppearance::palette() const
{
    QPalette none;
    none.resolve(0);
    return none;
}

#include "SystemAppearance_linux.moc"
