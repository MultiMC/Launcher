#pragma once

#include <QObject>
#include <memory>

class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
class BaseInstance;
typedef std::shared_ptr<BaseInstance> InstancePtr;

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
