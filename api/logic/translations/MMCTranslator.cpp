#include "MMCTranslator.h"

#include "qplatformdefs.h"

#include "qtranslator.h"

#include "qfileinfo.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qcoreapplication.h"
#include "qdatastream.h"
#include "qfile.h"
#include "qmap.h"
#include "qalgorithms.h"
#include "qlocale.h"
#include "qendian.h"
#include "qresource.h"

#include <QDebug>

enum Tag
{
    Tag_End = 1,
    Tag_SourceText16,
    Tag_Translation,
    Tag_Context16,
    Tag_Obsolete1,
    Tag_SourceText,
    Tag_Context,
    Tag_Comment,
    Tag_Obsolete2
};

// magic number for the file
static const int MagicLength = 16;
static const uchar magic[MagicLength] =
{
    0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95,
    0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd
};

enum {
    Q_EQ          = 0x01,
    Q_LT          = 0x02,
    Q_LEQ         = 0x03,
    Q_BETWEEN     = 0x04,
    Q_NOT         = 0x08,
    Q_MOD_10      = 0x10,
    Q_MOD_100     = 0x20,
    Q_LEAD_1000   = 0x40,
    Q_AND         = 0xFD,
    Q_OR          = 0xFE,
    Q_NEWRULE     = 0xFF,
    Q_OP_MASK     = 0x07,
    Q_NEQ         = Q_NOT | Q_EQ,
    Q_GT          = Q_NOT | Q_LEQ,
    Q_GEQ         = Q_NOT | Q_LT,
    Q_NOT_BETWEEN = Q_NOT | Q_BETWEEN
};

static inline QString dotQmLiteral() { return QStringLiteral(".qm"); }

static bool match(const uchar *found, uint foundLen, const char *target, uint targetLen)
{
    // catch the case if \a found has a zero-terminating symbol and \a len includes it.
    // (normalize it to be without the zero-terminating symbol)
    if (foundLen > 0 && found[foundLen-1] == '\0')
        --foundLen;
    return ((targetLen == foundLen) && memcmp(found, target, foundLen) == 0);
}

static void elfHash_continue(const char *name, uint &h)
{
    const uchar *k;
    uint g;

    k = (const uchar *) name;
    while (*k) {
        h = (h << 4) + *k++;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 24;
        h &= ~g;
    }
}

static void elfHash_finish(uint &h)
{
    if (!h)
        h = 1;
}

static uint elfHash(const char *name)
{
    uint hash = 0;
    elfHash_continue(name, hash);
    elfHash_finish(hash);
    return hash;
}

/*
   \internal

   Determines whether \a rules are valid "numerus rules". Test input with this
   function before calling numerusHelper, below.
 */
static bool isValidNumerusRules(const uchar *rules, uint rulesSize)
{
    // Disabled computation of maximum numerus return value
    // quint32 numerus = 0;

    if (rulesSize == 0)
        return true;

    quint32 offset = 0;
    do
    {
        uchar opcode = rules[offset];
        uchar op = opcode & Q_OP_MASK;

        if (opcode & 0x80)
            return false; // Bad op

        if (++offset == rulesSize)
            return false; // Missing operand

        // right operand
        ++offset;

        switch (op)
        {
        case Q_EQ:
        case Q_LT:
        case Q_LEQ:
        {
            break;
        }

        case Q_BETWEEN:
        {
            if (offset != rulesSize)
            {
                // third operand
                ++offset;
                break;
            }
            return false; // Missing operand
        }

        default:
        {
            return false; // Bad op (0)
        }
        }

        // ++numerus;
        if (offset == rulesSize)
            return true;

    } while (((rules[offset] == Q_AND) || (rules[offset] == Q_OR) || (rules[offset] == Q_NEWRULE)) && ++offset != rulesSize);

    // Bad op
    return false;
}

/*
   \internal

   This function does no validation of input and assumes it is well-behaved,
   these assumptions can be checked with isValidNumerusRules, above.

   Determines which translation to use based on the value of \a n. The return
   value is an index identifying the translation to be used.

   \a rules is a character array of size \a rulesSize containing bytecode that
   operates on the value of \a n and ultimately determines the result.

   This function has O(1) space and O(rulesSize) time complexity.
 */
static uint numerusHelper(int n, const uchar *rules, uint rulesSize)
{
    uint result = 0;
    uint i = 0;

    if (rulesSize == 0)
    {
        return 0;
    }

    for (;;)
    {
        bool orExprTruthValue = false;

        for (;;)
        {
            bool andExprTruthValue = true;

            for (;;) {
                bool truthValue = true;
                int opcode = rules[i++];

                int leftOperand = n;
                if (opcode & Q_MOD_10)
                {
                    leftOperand %= 10;
                }
                else if (opcode & Q_MOD_100)
                {
                    leftOperand %= 100;
                }
                else if (opcode & Q_LEAD_1000)
                {
                    while (leftOperand >= 1000)
                    {
                        leftOperand /= 1000;
                    }
                }

                int op = opcode & Q_OP_MASK;
                int rightOperand = rules[i++];

                switch (op)
                {
                case Q_EQ:
                    truthValue = (leftOperand == rightOperand);
                    break;
                case Q_LT:
                    truthValue = (leftOperand < rightOperand);
                    break;
                case Q_LEQ:
                    truthValue = (leftOperand <= rightOperand);
                    break;
                case Q_BETWEEN:
                    int bottom = rightOperand;
                    int top = rules[i++];
                    truthValue = (leftOperand >= bottom && leftOperand <= top);
                }

                if (opcode & Q_NOT)
                    truthValue = !truthValue;

                andExprTruthValue = andExprTruthValue && truthValue;

                if (i == rulesSize || rules[i] != Q_AND)
                {
                    break;
                }
                ++i;
            }

            orExprTruthValue = orExprTruthValue || andExprTruthValue;

            if (i == rulesSize || rules[i] != Q_OR)
                break;
            ++i;
        }

        if (orExprTruthValue)
            return result;

        ++result;

        if (i == rulesSize)
        {
            return result;
        }

        i++; // Q_NEWRULE
    }

    Q_ASSERT(false);
    return 0;
}

class MMCTranslatorPrivate
{
public:
    enum { Contexts = 0x2f, Hashes = 0x42, Messages = 0x69, NumerusRules = 0x88, Dependencies = 0x96 };

    MMCTranslatorPrivate() :
          unmapPointer(0), unmapLength(0), resource(0),
          messageArray(0), offsetArray(0), contextArray(0), numerusRulesArray(0),
          messageLength(0), offsetLength(0), contextLength(0), numerusRulesLength(0) {}

    char *unmapPointer;     // used memory (mmap, new or resource file)
    quint32 unmapLength;

    // The resource object in case we loaded the translations from a resource
    QResource *resource;

    // used if the translator has dependencies
    QList<MMCTranslator*> subTranslators;

    // Pointers and offsets into unmapPointer[unmapLength] array, or user
    // provided data array
    const uchar *messageArray;
    const uchar *offsetArray;
    const uchar *contextArray;
    const uchar *numerusRulesArray;
    uint messageLength;
    uint offsetLength;
    uint contextLength;
    uint numerusRulesLength;

    QString filename;

    bool do_load(const QString &filename, const QString &directory);
    bool do_load(const uchar *data, int len, const QString &directory);
    QString do_translate(const char *context, const char *sourceText, const char *comment, int n) const;
    void clear();
};

/*!
    Constructs an empty message file object with parent \a parent that
    is not connected to any file.
*/

MMCTranslator::MMCTranslator(QObject * parent)
    : QTranslator(parent)
{
    d = std::unique_ptr<MMCTranslatorPrivate>(new MMCTranslatorPrivate);
}

/*!
    Destroys the object and frees any allocated resources.
*/

MMCTranslator::~MMCTranslator()
{
    if (QCoreApplication::instance())
    {
        QCoreApplication::removeTranslator(this);
    }
    d->clear();
}

/*!

    Loads \a filename + \a suffix (".qm" if the \a suffix is not
    specified), which may be an absolute file name or relative to \a
    directory. Returns \c true if the translation is successfully loaded;
    otherwise returns \c false.

    If \a directory is not specified, the current directory is used
    (i.e., as \l{QDir::}{currentPath()}).

    The previous contents of this translator object are discarded.

    If the file name does not exist, other file names are tried
    in the following order:

    \list 1
    \li File name without \a suffix appended.
    \li File name with text after a character in \a search_delimiters
       stripped ("_." is the default for \a search_delimiters if it is
       an empty string) and \a suffix.
    \li File name stripped without \a suffix appended.
    \li File name stripped further, etc.
    \endlist

    For example, an application running in the fr_CA locale
    (French-speaking Canada) might call load("foo.fr_ca",
    "/opt/foolib"). load() would then try to open the first existing
    readable file from this list:

    \list 1
    \li \c /opt/foolib/foo.fr_ca.qm
    \li \c /opt/foolib/foo.fr_ca
    \li \c /opt/foolib/foo.fr.qm
    \li \c /opt/foolib/foo.fr
    \li \c /opt/foolib/foo.qm
    \li \c /opt/foolib/foo
    \endlist

    Usually, it is better to use the MMCTranslator::load(const QLocale &,
    const QString &, const QString &, const QString &, const QString &)
    function instead, because it uses \l{QLocale::uiLanguages()} and not simply
    the locale name, which refers to the formatting of dates and numbers and not
    necessarily the UI language.
*/

bool MMCTranslator::load(const QString & filename, const QString & directory, const QString & search_delimiters, const QString & suffix)
{
    d->clear();

    QString prefix;
    if (QFileInfo(filename).isRelative()) {
        prefix = directory;
        if (prefix.length() && !prefix.endsWith(QLatin1Char('/')))
            prefix += QLatin1Char('/');
    }

    const QString suffixOrDotQM = suffix.isNull() ? dotQmLiteral() : suffix;
    QStringRef fname(&filename);
    QString realname;
    const QString delims = search_delimiters.isNull() ? QStringLiteral("_.") : search_delimiters;

    for (;;)
    {
        QFileInfo fi;

        realname = prefix + fname + suffixOrDotQM;
        fi.setFile(realname);
        if (fi.isReadable() && fi.isFile())
        {
            break;
        }

        realname = prefix + fname;
        fi.setFile(realname);
        if (fi.isReadable() && fi.isFile())
        {
            break;
        }

        int rightmost = 0;
        for (int i = 0; i < (int)delims.length(); i++)
        {
            int k = fname.lastIndexOf(delims[i]);
            if (k > rightmost)
            {
                rightmost = k;
            }
        }

        // no truncations? fail
        if (rightmost == 0)
        {
            return false;
        }

        fname.truncate(rightmost);
    }

    qDebug() << "Loading a translation file from" << realname;

    // realname is now the fully qualified name of a readable file.
    return d->do_load(realname, directory);
}

bool MMCTranslatorPrivate::do_load(const QString &realname, const QString &directory)
{
    MMCTranslatorPrivate *d = this;
    bool ok = false;

    filename = realname;

    if (realname.startsWith(QLatin1Char(':')))
    {
        // If the translation is in a non-compressed resource file, the data is already in
        // memory, so no need to use QFile to copy it again.
        Q_ASSERT(!d->resource);
        d->resource = new QResource(realname);
        if (resource->isValid() && !resource->isCompressed() && resource->size() > MagicLength && !memcmp(resource->data(), magic, MagicLength))
        {
            d->unmapLength = resource->size();
            d->unmapPointer = reinterpret_cast<char *>(const_cast<uchar *>(resource->data()));
            ok = true;
        }
        else
        {
            delete resource;
            resource = 0;
        }
    }

    if (!ok)
    {
        QFile file(realname);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
        {
            return false;
        }

        qint64 fileSize = file.size();
        if (fileSize <= MagicLength || quint32(-1) <= fileSize)
        {
            return false;
        }

        {
            char magicBuffer[MagicLength];
            if (MagicLength != file.read(magicBuffer, MagicLength) || memcmp(magicBuffer, magic, MagicLength))
            {
                return false;
            }
        }

        d->unmapLength = quint32(fileSize);

        if (!ok)
        {
            d->unmapPointer = new char[d->unmapLength];
            if (d->unmapPointer)
            {
                file.seek(0);
                qint64 readResult = file.read(d->unmapPointer, d->unmapLength);
                if (readResult == qint64(unmapLength))
                {
                    ok = true;
                }
            }
        }
    }

    if (ok && d->do_load(reinterpret_cast<const uchar *>(d->unmapPointer), d->unmapLength, directory))
    {
        return true;
    }

    if (!d->resource)
    {
        delete [] unmapPointer;
    }

    delete d->resource;
    d->resource = 0;
    d->unmapPointer = 0;
    d->unmapLength = 0;

    return false;
}

Q_NEVER_INLINE
static bool is_readable_file(const QString &name)
{
    const QFileInfo fi(name);
    return fi.isReadable() && fi.isFile();
}

static QString find_translation(const QLocale & locale, const QString & filename, const QString & prefix, const QString & directory, const QString & suffix)
{
    QString path;
    if (QFileInfo(filename).isRelative()) {
        path = directory;
        if (!path.isEmpty() && !path.endsWith(QLatin1Char('/')))
            path += QLatin1Char('/');
    }
    const QString suffixOrDotQM = suffix.isNull() ? dotQmLiteral() : suffix;

    QString realname;
    realname += path + filename + prefix; // using += in the hope for some reserve capacity
    const int realNameBaseSize = realname.size();
    QStringList fuzzyLocales;

    // see http://www.unicode.org/reports/tr35/#LanguageMatching for inspiration

    QStringList languages = locale.uiLanguages();
#if defined(Q_OS_UNIX)
    for (int i = languages.size()-1; i >= 0; --i) {
        QString lang = languages.at(i);
        QString lowerLang = lang.toLower();
        if (lang != lowerLang)
            languages.insert(i+1, lowerLang);
    }
#endif

    // try explicit locales names first
    for (QString localeName : qAsConst(languages))
    {
        localeName.replace(QLatin1Char('-'), QLatin1Char('_'));

        realname += localeName + suffixOrDotQM;
        if (is_readable_file(realname))
            return realname;

        realname.truncate(realNameBaseSize + localeName.size());
        if (is_readable_file(realname))
            return realname;

        realname.truncate(realNameBaseSize);
        fuzzyLocales.append(localeName);
    }

    // start guessing
    for (const QString &fuzzyLocale : qAsConst(fuzzyLocales))
    {
        QStringRef localeName(&fuzzyLocale);
        for (;;)
        {
            int rightmost = localeName.lastIndexOf(QLatin1Char('_'));
            // no truncations? fail
            if (rightmost <= 0)
            {
                break;
            }
            localeName.truncate(rightmost);

            realname += localeName + suffixOrDotQM;
            if (is_readable_file(realname))
                return realname;

            realname.truncate(realNameBaseSize + localeName.size());
            if (is_readable_file(realname))
                return realname;

            realname.truncate(realNameBaseSize);
        }
    }

    const int realNameBaseSizeFallbacks = path.size() + filename.size();

    // realname == path + filename + prefix;
    if (!suffix.isNull())
    {
        realname.replace(realNameBaseSizeFallbacks, prefix.size(), suffix);
        // realname == path + filename;
        if (is_readable_file(realname))
            return realname;
        realname.replace(realNameBaseSizeFallbacks, suffix.size(), prefix);
    }

    // realname == path + filename + prefix;
    if (is_readable_file(realname))
        return realname;

    realname.truncate(realNameBaseSizeFallbacks);
    // realname == path + filename;
    if (is_readable_file(realname))
        return realname;

    realname.truncate(0);
    return realname;
}

/*!
    \since 4.8

    Loads \a filename + \a prefix + \l{QLocale::uiLanguages()}{ui language
    name} + \a suffix (".qm" if the \a suffix is not specified), which may be
    an absolute file name or relative to \a directory. Returns \c true if the
    translation is successfully loaded; otherwise returns \c false.

    The previous contents of this translator object are discarded.

    If the file name does not exist, other file names are tried
    in the following order:

    \list 1
    \li File name without \a suffix appended.
    \li File name with ui language part after a "_" character stripped and \a suffix.
    \li File name with ui language part stripped without \a suffix appended.
    \li File name with ui language part stripped further, etc.
    \endlist

    For example, an application running in the \a locale with the following
    \l{QLocale::uiLanguages()}{ui languages} - "es", "fr-CA", "de" might call
    load(QLocale(), "foo", ".", "/opt/foolib", ".qm"). load() would
    replace '-' (dash) with '_' (underscore) in the ui language and then try to
    open the first existing readable file from this list:

    \list 1
    \li \c /opt/foolib/foo.es.qm
    \li \c /opt/foolib/foo.es
    \li \c /opt/foolib/foo.fr_CA.qm
    \li \c /opt/foolib/foo.fr_CA
    \li \c /opt/foolib/foo.de.qm
    \li \c /opt/foolib/foo.de
    \li \c /opt/foolib/foo.fr.qm
    \li \c /opt/foolib/foo.fr
    \li \c /opt/foolib/foo.qm
    \li \c /opt/foolib/foo.
    \li \c /opt/foolib/foo
    \endlist

    On operating systems where file system is case sensitive, MMCTranslator also
    tries to load a lower-cased version of the locale name.
*/
bool MMCTranslator::load(const QLocale & locale, const QString & filename, const QString & prefix, const QString & directory, const QString & suffix)
{
    d->clear();
    QString fname = find_translation(locale, filename, prefix, directory, suffix);
    return !fname.isEmpty() && d->do_load(fname, directory);
}

/*!
  \overload load()

  Loads the QM file data \a data of length \a len into the
  translator.

  The data is not copied. The caller must be able to guarantee that \a data
  will not be deleted or modified.

  \a directory is only used to specify the base directory when loading the dependencies
  of a QM file. If the file does not have dependencies, this argument is ignored.
*/
bool MMCTranslator::load(const uchar *data, int len, const QString &directory)
{
    d->clear();

    if (!data || len < MagicLength || memcmp(data, magic, MagicLength))
    {
        return false;
    }

    return d->do_load(data, len, directory);
}

static quint8 read8(const uchar *data)
{
    return qFromBigEndian<quint8>(data);
}

static quint16 read16(const uchar *data)
{
    return qFromBigEndian<quint16>(data);
}

static quint32 read32(const uchar *data)
{
    return qFromBigEndian<quint32>(data);
}

bool MMCTranslatorPrivate::do_load(const uchar *data, int len, const QString &directory)
{
    bool ok = true;
    const uchar *end = data + len;

    data += MagicLength;

    QStringList dependencies;
    while (data < end - 4)
    {
        quint8 tag = read8(data++);
        quint32 blockLen = read32(data);
        data += 4;
        if (!tag || !blockLen)
        {
            break;
        }
        if (quint32(end - data) < blockLen)
        {
            ok = false;
            break;
        }

        if (tag == MMCTranslatorPrivate::Contexts)
        {
            contextArray = data;
            contextLength = blockLen;
        }
        else if (tag == MMCTranslatorPrivate::Hashes)
        {
            offsetArray = data;
            offsetLength = blockLen;
        }
        else if (tag == MMCTranslatorPrivate::Messages)
        {
            messageArray = data;
            messageLength = blockLen;
        }
        else if (tag == MMCTranslatorPrivate::NumerusRules)
        {
            numerusRulesArray = data;
            numerusRulesLength = blockLen;
        }
        else if (tag == MMCTranslatorPrivate::Dependencies)
        {
            QDataStream stream(QByteArray::fromRawData((const char*)data, blockLen));
            QString dep;
            while (!stream.atEnd())
            {
                stream >> dep;
                dependencies.append(dep);
            }
        }

        data += blockLen;
    }

    if (dependencies.isEmpty() && (!offsetArray || !messageArray))
    {
        ok = false;
    }
    if (ok && !isValidNumerusRules(numerusRulesArray, numerusRulesLength))
    {
        ok = false;
    }
    if (ok)
    {
        const int dependenciesCount = dependencies.count();
        subTranslators.reserve(dependenciesCount);
        for (int i = 0 ; i < dependenciesCount; ++i)
        {
            MMCTranslator *translator = new MMCTranslator;
            subTranslators.append(translator);
            ok = translator->load(dependencies.at(i), directory);
            if (!ok)
            {
                break;
            }
        }

        // In case some dependencies fail to load, unload all the other ones too.
        if (!ok)
        {
            qDeleteAll(subTranslators);
            subTranslators.clear();
        }
    }

    if (!ok)
    {
        messageArray = 0;
        contextArray = 0;
        offsetArray = 0;
        numerusRulesArray = 0;
        messageLength = 0;
        contextLength = 0;
        offsetLength = 0;
        numerusRulesLength = 0;
    }

    return ok;
}

static QString getMessage(const uchar *m, const uchar *end, const char *context, const char *sourceText, const char *comment, uint numerus)
{
    const uchar *tn = 0;
    uint tn_length = 0;
    const uint sourceTextLen = uint(strlen(sourceText));
    const uint contextLen = uint(strlen(context));
    const uint commentLen = uint(strlen(comment));

    for (;;)
    {
        uchar tag = 0;
        if (m < end)
        {
            tag = read8(m++);
        }
        switch((Tag)tag)
        {
        case Tag_End:
        {
            goto end;
        }
        case Tag_Translation:
        {
            int len = read32(m);
            if (len % 1)
            {
                return QString();
            }
            m += 4;
            if (!numerus--)
            {
                tn_length = len;
                tn = m;
            }
            m += len;
            break;
        }
        case Tag_Obsolete1:
        {
            m += 4;
            break;
        }
        case Tag_SourceText:
        {
            quint32 len = read32(m);
            m += 4;
            if (!match(m, len, sourceText, sourceTextLen))
                return QString();
            m += len;
            break;
        }
        case Tag_Context:
        {
            quint32 len = read32(m);
            m += 4;
            if (!match(m, len, context, contextLen))
                return QString();
            m += len;
            break;
        }
        case Tag_Comment:
        {
            quint32 len = read32(m);
            m += 4;
            if (*m && !match(m, len, comment, commentLen))
                return QString();
            m += len;
            break;
        }
        default:
        {
            return QString();
        }
        }
    }
end:
    if (!tn)
    {
        return QString();
    }
    QString str = QString((const QChar *)tn, tn_length/2);
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian)
    {
        for (int i = 0; i < str.length(); ++i)
        {
            str[i] = QChar((str.at(i).unicode() >> 8) + ((str.at(i).unicode() << 8) & 0xff00));
        }
    }
    return str;
}

QString MMCTranslatorPrivate::do_translate(const char *context, const char *sourceText, const char *comment, int n) const
{
    if (context == 0)
    {
        context = "";
    }
    if (sourceText == 0)
    {
        sourceText = "";
    }
    if (comment == 0)
    {
        comment = "";
    }

    uint numerus = 0;
    size_t numItems = 0;

    if (!offsetLength)
    {
        qDebug() << sourceText << "offsetLength is 0, searching deps" << filename;
        goto searchDependencies;
    }

    /*
        Check if the context belongs to this MMCTranslator. If many
        translators are installed, this step is necessary.
    */
    if (contextLength)
    {
        quint16 hTableSize = read16(contextArray);
        uint g = elfHash(context) % hTableSize;
        const uchar *c = contextArray + 2 + (g << 1);
        quint16 off = read16(c);
        c += 2;
        if (off == 0)
        {
            qDebug() << sourceText << "off == 0:" << filename;
            return QString();
        }
        c = contextArray + (2 + (hTableSize << 1) + (off << 1));

        const uint contextLen = uint(strlen(context));
        for (;;)
        {
            quint8 len = read8(c++);
            if (len == 0)
            {
                qDebug() << sourceText << "len == 0:" << filename;
                return QString();
            }
            if (match(c, len, context, contextLen))
            {
                break;
            }
            c += len;
        }
    }

    numItems = offsetLength / (2 * sizeof(quint32));
    if (!numItems)
    {
        qDebug() << sourceText << "numItems is 0, searching deps" << filename;
        goto searchDependencies;
    }

    if (n >= 0)
    {
        numerus = numerusHelper(n, numerusRulesArray, numerusRulesLength);
    }

    for (;;)
    {
        quint32 h = 0;
        elfHash_continue(sourceText, h);
        elfHash_continue(comment, h);
        elfHash_finish(h);

        const uchar *start = offsetArray;
        const uchar *end = start + ((numItems-1) << 3);
        while (start <= end)
        {
            const uchar *middle = start + (((end - start) >> 4) << 3);
            uint hash = read32(middle);
            if (h == hash)
            {
                start = middle;
                qDebug() << sourceText << "maybe has a hash in" << filename;
                break;
            }
            else if (hash < h)
            {
                start = middle + 8;
            }
            else
            {
                end = middle - 8;
            }
        }

        if (start <= end)
        {
            // go back on equal key
            while (start != offsetArray && read32(start) == read32(start-8))
            {
                start -= 8;
            }

            while (start < offsetArray + offsetLength)
            {
                quint32 rh = read32(start);
                start += 4;
                if (rh != h)
                {
                    qDebug() << sourceText << "does not have a hash in" << filename;
                    break;
                }
                quint32 ro = read32(start);
                start += 4;
                QString tn = getMessage(messageArray + ro, messageArray + messageLength, context, sourceText, comment, numerus);
                if (!tn.isNull())
                {
                    qDebug() << sourceText << "read from" << filename << ":" << tn;
                    return tn;
                }
                else
                {
                    qDebug() << sourceText << "not read from" << filename;
                }
            }
        }
        if (!comment[0])
        {
            break;
        }
        comment = "";
    }

searchDependencies:
    for (MMCTranslator *translator : subTranslators)
    {
        QString tn = translator->translate(context, sourceText, comment, n);
        if (!tn.isNull())
        {
            return tn;
        }
    }
    qDebug() << sourceText << "Was ultimately not found in" << filename;
    return QString();
}

/*
    Empties this translator of all contents.

    This function works with stripped translator files.
*/

void MMCTranslatorPrivate::clear()
{
    if (unmapPointer && unmapLength)
    {
        if (!resource)
        {
            delete [] unmapPointer;
        }
    }

    delete resource;
    resource = 0;
    unmapPointer = 0;
    unmapLength = 0;
    messageArray = 0;
    contextArray = 0;
    offsetArray = 0;
    numerusRulesArray = 0;
    messageLength = 0;
    contextLength = 0;
    offsetLength = 0;
    numerusRulesLength = 0;

    qDeleteAll(subTranslators);
    subTranslators.clear();

    // FIXME: this is shit.
    /*
    if (QCoreApplicationPrivate::isTranslatorInstalled(q))
        QCoreApplication::postEvent(QCoreApplication::instance(),
                                    new QEvent(QEvent::LanguageChange));
    */
}

/*!
    Returns the translation for the key (\a context, \a sourceText,
    \a disambiguation). If none is found, also tries (\a context, \a
    sourceText, ""). If that still fails, returns a null string.

    \note Incomplete translations may result in unexpected behavior:
    If no translation for (\a context, \a sourceText, "")
    is provided, the method might in this case actually return a
    translation for a different \a disambiguation.

    If \a n is not -1, it is used to choose an appropriate form for
    the translation (e.g. "%n file found" vs. "%n files found").

    If you need to programatically insert translations into a
    MMCTranslator, this function can be reimplemented.

    \sa load()
*/
QString MMCTranslator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    return d->do_translate(context, sourceText, disambiguation, n);
}

/*!
    Returns \c true if this translator is empty, otherwise returns \c false.
    This function works with stripped and unstripped translation files.
*/
bool MMCTranslator::isEmpty() const
{
    return !d->unmapPointer && !d->unmapLength && !d->messageArray && !d->offsetArray && !d->contextArray && d->subTranslators.isEmpty();
}
