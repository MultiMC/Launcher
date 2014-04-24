#pragma once

#include <QObject>
#include <QHash>
#include <memory>

#include <logic/BaseInstance.h>

class QWidget;

class BaseInstance;
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

	QList<QuickModVersionPtr> resolve(const QList<QuickMod *> &mods);

signals:
	void error(const QString &message);
	void warning(const QString &message);
	void success(const QString &message);

private:
	QWidget *m_widgetParent;
	InstancePtr m_instance;

	QHash<QuickMod *, QuickModVersionPtr> m_mods;

	QuickModVersionPtr getVersion(QuickMod *mod, const QString &filter, bool *ok);
	/// \param exclude Used for telling which versions are already included, so they won't have
	/// to be added or checked now
	void resolve(const QuickModVersionPtr version);
};
