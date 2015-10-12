#pragma once

#include "BaseVersion.h"
#include "BaseWonkoEntity.h"

#include <QVector>
#include <memory>

#include "WonkoReference.h"

#include "multimc_logic_export.h"

using WonkoVersionPtr = std::shared_ptr<class WonkoVersion>;

class MULTIMC_LOGIC_EXPORT WonkoVersion : public QObject, public BaseVersion, public BaseWonkoEntity
{
	Q_OBJECT
	Q_PROPERTY(QString uid READ uid CONSTANT)
	Q_PROPERTY(QString version READ version CONSTANT)
	Q_PROPERTY(QString type READ type NOTIFY typeChanged)
	Q_PROPERTY(QDateTime time READ time NOTIFY timeChanged)
	Q_PROPERTY(QVector<WonkoReference> requires READ requires NOTIFY requiresChanged)
public:
	explicit WonkoVersion(const QString &uid, const QString &version);

	QString descriptor() override;
	QString name() override;
	QString typeString() const override;

	QString uid() const { return m_uid; }
	QString version() const { return m_version; }
	QString type() const { return m_type; }
	QDateTime time() const;
	qint64 rawTime() const { return m_time; }
	QVector<WonkoReference> requires() const { return m_requires; }

	// data accessors
	QString mainClass() const { return m_mainClass; }
	QString appletClass() const { return m_appletClass; }
	QString assets() const { return m_assets; }
	QString minecraftArguments() const { return m_minecraftArguments; }
	QStringList tweakers() const { return m_tweakers; }
	QVector<QJsonObject> libraries() const { return m_libraries; }
	QVector<QJsonObject> jarMods() const { return m_jarMods; }

	Task *remoteUpdateTask() override;
	Task *localUpdateTask() override;
	void merge(const std::shared_ptr<BaseWonkoEntity> &other) override;

	QString localFilename() const override;
	QJsonObject serialized() const override;

public: // for usage by format parsers only
	void setType(const QString &type);
	void setTime(const qint64 time);
	void setRequires(const QVector<WonkoReference> &requires);
	void setData(const QString &mainClass, const QString &appletClass, const QString &assets,
				 const QString &minecraftArguments, const QStringList &tweakers,
				 const QVector<QJsonObject> &libraries, const QVector<QJsonObject> &jarMods);

signals:
	void typeChanged();
	void timeChanged();
	void requiresChanged();

private:
	QString m_uid;
	QString m_version;
	QString m_type;
	qint64 m_time;
	QVector<WonkoReference> m_requires;

private: // actual data fields
	QString m_mainClass;
	QString m_appletClass;
	QString m_assets;
	QString m_minecraftArguments;
	QStringList m_tweakers;
	QVector<QJsonObject> m_libraries;
	QVector<QJsonObject> m_jarMods;
};

Q_DECLARE_METATYPE(WonkoVersionPtr)
