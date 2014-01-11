#pragma once

#include <QObject>
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

	QList<QuickModVersionPtr> resolve(const QList<QuickMod *> &mods);

signals:
	void totallyUnknownError();
	void didNotSelectVersionError(QuickModVersionPtr from, const QString &to);
	void unresolvedDependency(QuickModVersionPtr from, const QString &to);
	void resolvedDependency(QuickModVersionPtr from, QuickModVersionPtr to);

private:
	QWidget *m_widgetParent;
	BaseInstance *m_instance;

	QuickModVersionPtr getVersion(QuickMod *mod, const QString &filter = QString());
	/// \param exclude Used for telling which versions are already included, so they won't have
	/// to be added or checked now
	QList<QuickModVersionPtr> resolve(const QuickModVersionPtr mod,
									  const QList<QuickModVersionPtr> &exclude);
};
