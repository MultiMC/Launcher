#pragma once

#include <QObject>
#include <memory>

#include <logic/BaseInstance.h>

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
class BaseInstance;

/**
 * Non-gui backend for QuickModInstallDialog
 */
class QuickModInstaller : public QObject
{
	Q_OBJECT
public:
	explicit QuickModInstaller(QWidget *widgetParent, QObject *parent = 0);

	void install(const QuickModVersionPtr version, InstancePtr instance);

	void handleDownload(QuickModVersionPtr version, const QByteArray &data, const QUrl &url);

private:
	QWidget *m_widgetParent;
};
