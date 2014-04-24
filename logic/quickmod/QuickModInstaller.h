#pragma once

#include <QObject>
#include <memory>

#include <logic/BaseInstance.h>

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
class BaseInstance;

class QuickModInstaller : public QObject
{
	Q_OBJECT
public:
	explicit QuickModInstaller(QWidget *widgetParent, QObject *parent = 0);

	bool install(const QuickModVersionPtr version, InstancePtr instance, QString *errorString);

private:
	QWidget *m_widgetParent;
};
