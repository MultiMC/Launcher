#pragma once

#include <QObject>
#include <QHash>
#include <memory>

#include <logic/quickmod/QuickMod.h>

class QWidget;

class BaseInstance;
class QuickModVersion;
class QuickMod;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDependencyResolver : public QObject
{
	Q_OBJECT
public:
	explicit QuickModDependencyResolver(BaseInstance* instance, QWidget *parent = 0);
	explicit QuickModDependencyResolver(BaseInstance* instance, QWidget *widgetParent,
										QObject *parent);

	QList<QuickModVersionPtr> resolve(const QList<QuickModPtr> &mods);

signals:
	void error(const QString &message);
	void warning(const QString &message);
	void success(const QString &message);

private:
	QWidget *m_widgetParent;
	BaseInstance* m_instance;

	QHash<QuickMod *, QuickModVersionPtr> m_mods;

	QuickModVersionPtr getVersion(QuickModPtr mod, const QString &filter, bool *ok);
	/// \param exclude Used for telling which versions are already included, so they won't have
	/// to be added or checked now
	void resolve(const QuickModVersionPtr version);
};
