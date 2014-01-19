#pragma once

#include <QObject>
#include <memory>

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
class BaseInstance;

class QuickModInstaller : public QObject
{
	Q_OBJECT
public:
	explicit QuickModInstaller(QWidget *widgetParent, QObject *parent = 0);

	bool install(const QuickModVersionPtr version, BaseInstance *instance, QString *errorString);

private:
	QWidget *m_widgetParent;
};
