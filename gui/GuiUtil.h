#pragma once

#include <QWidget>
#include <memory>

class ProgressProvider;
class Bindable;

class GuiUtil : public QObject
{
	Q_OBJECT
public:
	static void uploadPaste(const QString &text, QWidget *parentWidget);
	static void setClipboardText(const QString &text);

	static void setup(Bindable *bindable, QWidget *widgetParent);

private slots:
	int progressDialog(ProgressProvider *provider);

private:
	QWidget *m_widgetParent;
};
