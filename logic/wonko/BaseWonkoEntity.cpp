#include "BaseWonkoEntity.h"

#include "Json.h"
#include "WonkoUtil.h"

BaseWonkoEntity::~BaseWonkoEntity()
{
}

void BaseWonkoEntity::store() const
{
	Json::write(serialized(), Wonko::localWonkoDir().absoluteFilePath(localFilename()));
}

void BaseWonkoEntity::notifyLocalLoadComplete()
{
	m_localLoaded = true;
	store();
}
void BaseWonkoEntity::notifyRemoteLoadComplete()
{
	m_remoteLoaded = true;
	store();
}
