#include "AuthProviders.h"
#include "providers/ElybyAuthProvider.h"
#include "providers/DummyAuthProvider.h"
#include "providers/MojangAuthProvider.h"

#define REGISTER_AUTH_PROVIDER(Provider)          \
  {                                               \
    AuthProviderPtr provider(new Provider());     \
    m_providers.insert(provider->id(), provider); \
  }

namespace AuthProviders
{
    QMap<QString, AuthProviderPtr> m_providers;

    void load()
    {
        REGISTER_AUTH_PROVIDER(ElybyAuthProvider);
        REGISTER_AUTH_PROVIDER(DummyAuthProvider);
        REGISTER_AUTH_PROVIDER(MojangAuthProvider);
    }

    AuthProviderPtr lookup(QString id)
    {
        qDebug() << "LOOKUP AUTH_PROVIDER" << id;
        if (m_providers.contains(id))
            return m_providers.value(id);

        qDebug() << "Lookup failed";
        return nullptr;
    }

    QList<AuthProviderPtr> getAll() {
        return m_providers.values();
    }
}
