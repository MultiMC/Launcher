#include <QTest>

#include "TestUtil.h"
#include "updater/v2/Signature.h"

/*
 * testdata/keys-1.json is signed with a throwaway key, made and used like this:
 *
 *   openssl genpkey -algorithm ed25519 -out test.key
 *   openssl pkey -in test.key -pubout -outform DER | tail -c 32 | xxd -p -c 64
 *   openssl pkeyutl -sign -rawin -inkey test.key -in keys-1.json -out keys-1.sig
 *   xxd -p -c 128 keys-1.sig
 *
 * The private key is not kept. The other key is from a second `openssl genpkey`.
 */
static const char *testKey = "dfd76e4677ecfcf2ac39bd44dc5a321d6d8edfb8184326cd7c55d8221073588e";
static const char *otherKey = "ad139b0d1155d60b682ab06373fb832a057615c88e7c61405528364f1562ba20";
static const char *testSignature =
    "7065246f66567a1048947b086bbcb4a3366b7edc243d1581a26a7b0f3495d82e"
    "fca8c33287f02113fbcf4105ee22d83fd2ba54417062fdf951bcc932a6e04b0c";

class SignatureTest : public QObject
{
    Q_OBJECT

    QByteArray signedFile()
    {
        return GET_TEST_FILE("data/keys-1.json");
    }

private
slots:
    // the signature covers the file exactly as it is in the repository (LF line endings)
    void test_signedFileIntact()
    {
        QCOMPARE(signedFile().size(), 277);
    }

    void test_opensslSignature()
    {
        QVERIFY(Signature::checkEd25519(signedFile(), QByteArray::fromHex(testSignature), QByteArray::fromHex(testKey)));
    }

    void test_modifiedFile_data()
    {
        QTest::addColumn<int>("position");
        QTest::newRow("first byte") << 0;
        QTest::newRow("middle") << 138;
        QTest::newRow("final newline") << 276;
    }
    void test_modifiedFile()
    {
        QFETCH(int, position);
        auto data = signedFile();
        QVERIFY(position < data.size());
        data[position] = data[position] ^ 0x01;
        QVERIFY(!Signature::checkEd25519(data, QByteArray::fromHex(testSignature), QByteArray::fromHex(testKey)));
    }

    void test_truncatedOrExtendedFile()
    {
        auto data = signedFile();
        auto signature = QByteArray::fromHex(testSignature);
        auto key = QByteArray::fromHex(testKey);
        QVERIFY(!Signature::checkEd25519(data.left(data.size() - 1), signature, key));
        QVERIFY(!Signature::checkEd25519(data + "\n", signature, key));
        QVERIFY(!Signature::checkEd25519(QByteArray(), signature, key));
    }

    void test_modifiedSignature_data()
    {
        QTest::addColumn<int>("position");
        QTest::newRow("R, first byte") << 0;
        QTest::newRow("R, last byte") << 31;
        QTest::newRow("S, first byte") << 32;
        QTest::newRow("S, last byte") << 63;
    }
    void test_modifiedSignature()
    {
        QFETCH(int, position);
        auto signature = QByteArray::fromHex(testSignature);
        signature[position] = signature[position] ^ 0x01;
        QVERIFY(!Signature::checkEd25519(signedFile(), signature, QByteArray::fromHex(testKey)));
    }

    void test_otherKey()
    {
        QVERIFY(!Signature::checkEd25519(signedFile(), QByteArray::fromHex(testSignature), QByteArray::fromHex(otherKey)));
    }

    void test_wrongSizes()
    {
        auto data = signedFile();
        auto signature = QByteArray::fromHex(testSignature);
        auto key = QByteArray::fromHex(testKey);
        QVERIFY(!Signature::checkEd25519(data, signature.left(63), key));
        QVERIFY(!Signature::checkEd25519(data, signature + '\0', key));
        QVERIFY(!Signature::checkEd25519(data, signature, key.left(31)));
        QVERIFY(!Signature::checkEd25519(data, signature, key + '\0'));
        QVERIFY(!Signature::checkEd25519(data, QByteArray(), QByteArray()));
        // a hex signature or key is not accepted in place of the raw bytes
        QVERIFY(!Signature::checkEd25519(data, QByteArray(testSignature), key));
        QVERIFY(!Signature::checkEd25519(data, signature, QByteArray(testKey)));
    }

    // RFC 8032, section 7.1: the standard Ed25519, not Monocypher's default EdDSA with BLAKE2b
    void test_rfc8032_data()
    {
        QTest::addColumn<QByteArray>("key");
        QTest::addColumn<QByteArray>("message");
        QTest::addColumn<QByteArray>("signature");
        QTest::newRow("TEST 1")
            << QByteArray::fromHex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a")
            << QByteArray()
            << QByteArray::fromHex(
                   "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
        QTest::newRow("TEST 2")
            << QByteArray::fromHex("3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c")
            << QByteArray::fromHex("72")
            << QByteArray::fromHex(
                   "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
    }
    void test_rfc8032()
    {
        QFETCH(QByteArray, key);
        QFETCH(QByteArray, message);
        QFETCH(QByteArray, signature);
        QVERIFY(Signature::checkEd25519(message, signature, key));
        QVERIFY(!Signature::checkEd25519(message + "x", signature, key));
    }
};

QTEST_GUILESS_MAIN(SignatureTest)

#include "Signature_test.moc"
