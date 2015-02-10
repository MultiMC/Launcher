#pragma once
#include "minecraft/ProfileStrategy.h"

class OneSixInstance;

class OneSixProfileStrategy : public ProfileStrategy
{
public:
	struct PatchItem
	{
		QString uid;
		QString version;
	};
	struct LoadOrder
	{
		bool insert(PatchItem item, const char * reason = 0)
		{
			if(seen(item.uid))
			{
				qCritical() << "Already seen" << item.uid << "version" << item.version << (reason ?(QString("reason %1").arg(reason)): "for no reason");
				return false;
			}
			qDebug() << "Adding" << item.uid << "version" << item.version << (reason ?(QString("reason %1").arg(reason)): "for no reason");
			loadOrder.append(item);
			index[item.uid] = &loadOrder.last();
			return false;
		}
		bool seen(QString uid) const
		{
			return index.contains(uid);
		}
		QList <PatchItem> loadOrder;
		QMap <QString, PatchItem *> index;
	};
public:
	OneSixProfileStrategy(OneSixInstance * instance);
	virtual ~OneSixProfileStrategy() {};
	virtual void load() override;
	virtual bool saveOrder(ProfileUtils::PatchOrder order) override;
	virtual bool installJarMods(QStringList filepaths) override;
	virtual bool removePatch(PackagePtr patch) override;

protected:
	void loadBuiltinPatch(QString uid, QString version);
	void loadLoosePatch(QString uid, QString name);

	void upgradeDeprecatedFiles();
protected:
	OneSixInstance *m_instance;
};
