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
	explicit QuickModDependencyResolver(BaseInstance *instance, QWidget *widgetParent, QObject *parent);

	QList<QuickModVersionPtr> resolve(const QList<QuickMod *> &mods);

private:
	QWidget *m_widgetParent;
	BaseInstance *m_instance;
};
