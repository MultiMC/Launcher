#pragma once

#include <QObject>
#include <QHash>
#include <memory>

class QWidget;

class BaseInstance;
class QuickModVersion;
class QuickMod;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDependencyResolver : public QObject
{
	Q_OBJECT
public:
	explicit QuickModDependencyResolver(BaseInstance *instance, QWidget *parent = 0);
	explicit QuickModDependencyResolver(BaseInstance *instance, QWidget *widgetParent,
										QObject *parent);

	QSet<QuickModVersionPtr> resolve(const QList<QuickMod *> &mods);

signals:
	void error(const QString &message);
	void warning(const QString &message);
	void success(const QString &message);

private:
	QWidget *m_widgetParent;
	BaseInstance *m_instance;

	enum Type
	{
		Explicit,
		Implicit
	};
	QHash<QuickMod *, QPair<QSet<QuickModVersionPtr>, Type> > m_mods;

	void addDependencies(QuickMod *mod);

	// query functions
	bool isUsed(const QuickMod *mod) const;
	bool isLatest(const QuickModVersionPtr &ptr, const QSet<QuickModVersionPtr> &set) const;
	// end query functions

	// other functions
	void reduce();
	void applyVersionFilter(QuickMod *mod, const QString &filter);
	void useLatest();
	// end other functions

	QuickModVersionPtr getVersion(QuickMod *mod, const QString &filter, bool *ok);
};
