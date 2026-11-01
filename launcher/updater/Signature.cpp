#include "Signature.h"

#include <monocypher-ed25519.h>

bool Signature::checkEd25519(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey)
{
    if(signature.size() != 64 || publicKey.size() != 32)
    {
        return false;
    }
    auto sig = reinterpret_cast<const uint8_t *>(signature.constData());
    auto key = reinterpret_cast<const uint8_t *>(publicKey.constData());
    auto message = reinterpret_cast<const uint8_t *>(data.constData());
    return crypto_ed25519_check(sig, key, message, size_t(data.size())) == 0;
}
