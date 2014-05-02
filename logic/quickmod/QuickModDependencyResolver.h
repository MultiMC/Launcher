#pragma once

#include <QObject>
#include <QHash>
#include <memory>

#include <logic/quickmod/QuickMod.h>

class QWidget;

class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;
class QuickModVersion;
class QuickMod;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDependencyResolver : public QObject
{
	Q_OBJECT
public:
	explicit QuickModDependencyResolver(InstancePtr instance, QWidget *parent = 0);
	explicit QuickModDependencyResolver(InstancePtr instance, QWidget *widgetParent,
										QObject *parent);

	QList<QuickModVersionPtr> resolve(const QList<QuickModUid> &mods);

signals:
	void error(const QString &message);
	void warning(const QString &message);
	void success(const QString &message);

private:
	QWidget *m_widgetParent;
	InstancePtr m_instance;

	QHash<QuickMod *, QuickModVersionPtr> m_mods;

	QuickModVersionPtr getVersion(const QuickModUid &modUid, const QString &filter, bool *ok);
	/// \param exclude Used for telling which versions are already included, so they won't have
	/// to be added or checked now
	void resolve(const QuickModVersionPtr version);
};
