#pragma once
#include <QByteArray>

namespace Signature
{

/**
 * Checks a detached Ed25519 signature over the exact bytes of `data`.
 *
 * This is the standard Ed25519 (RFC 8032, not the prehashed variant), the same one
 * `openssl pkeyutl -sign -rawin` makes. `signature` is the raw 64 bytes, `publicKey` the
 * raw 32 bytes. Anything of the wrong size fails.
 */
bool checkEd25519(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey);

}
