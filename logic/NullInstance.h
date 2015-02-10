#include "BaseInstance.h"

class NullInstance: public BaseInstance
{
public:
	NullInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir)
	:BaseInstance(globalSettings, settings, rootDir)
	{
		setFlag(BaseInstance::VersionBrokenFlag);
	}
	virtual ~NullInstance() {};
	virtual void cleanupAfterRun() override
	{
	}
	virtual void init() override
	{
	};
	virtual QString getStatusbarDescription() override
	{
		return tr("Unknown instance type");
	};
	virtual QSet< QString > traits()
	{
		return {};
	};
	virtual QString instanceConfigFolder() const
	{
		return instanceRoot();
	};
	virtual BaseProcess* prepareForLaunch(AuthSessionPtr)
	{
		return nullptr;
	}
	virtual std::shared_ptr< Task > doUpdate()
	{
		return nullptr;
	}
};
