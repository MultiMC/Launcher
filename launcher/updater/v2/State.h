#pragma once
#include <QByteArray>
#include <QString>

#include "Formats.h"

namespace UpdaterV2
{

/**
 * What updater v2 has verified so far, kept between runs: the highest tags serial, the
 * newest keys document and the newest mirror list. Each build has floors compiled in (a keys
 * document and a serial); when the stored state is lost or broken, the floors take its place.
 *
 * The stored file is the Launcher's own, not part of the tree. It holds the documents as the
 * bytes that were verified, and is only written after they were. It also holds the pinned root
 * keys it was written with: a stored state from a build pinned to other root keys (another
 * tree, such as a test setup's) is ignored, or its serial and keys would fail every check.
 */
class State
{
public:
    /**
     * `floorKeys` is the compiled-in keys document, trusted without a signature. It must be
     * valid and list exactly the pinned root keys `pinnedRootKeys`, or error() says why and
     * nothing is trusted.
     */
    State(const QByteArray &floorKeys, const QStringList &pinnedRootKeys, qint64 serialFloor);

    // empty when the floors are usable
    QString error() const
    {
        return m_error;
    }

    /**
     * Read the stored state. Anything missing or broken leaves the floors in place.
     */
    void load(const QString &path);
    bool save(const QString &path) const;

    const KeysDocument &keys() const
    {
        return m_keys;
    }
    qint64 serial() const
    {
        return m_serial;
    }
    bool hasMirrors() const
    {
        return !m_mirrorsData.isEmpty();
    }
    const MirrorsDocument &mirrors() const
    {
        return m_mirrors;
    }

    /**
     * Take a newly verified document. Lower keys versions, lower serials and older mirror
     * lists are ignored. Return true when something changed.
     */
    bool acceptKeys(const KeysDocument &keys, const QByteArray &data);
    bool acceptSerial(qint64 serial);
    bool acceptMirrors(const MirrorsDocument &mirrors, const QByteArray &data);

private:
    QString m_error;
    KeysDocument m_floorKeys;
    QStringList m_pinnedRootKeys;
    qint64 m_serialFloor = 0;

    KeysDocument m_keys;
    QByteArray m_keysData;
    qint64 m_serial = 0;
    MirrorsDocument m_mirrors;
    QByteArray m_mirrorsData;
};

}
