#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "global_def.hpp"

class QWidget;                          class QQmlEngine;
class QJSEngine;

DECL_PLUG_CHANGED_T(QComboBox, currentIndexChanged, int)
DECL_PLUG_CHANGED  (QAbstractButton, toggled)
DECL_PLUG_CHANGED  (QLineEdit, textChanged)
DECL_PLUG_CHANGED_T(QButtonGroup, buttonToggled, int, bool)
DECL_PLUG_CHANGED_T(QDoubleSpinBox, valueChanged, double)
DECL_PLUG_CHANGED_T(QSpinBox, valueChanged, int)

namespace Global {

auto _SetWindowTitle(QWidget *w, const QString &title) -> void;

namespace pch_detail {
template<class T>
struct GenericEq {
    static auto eq(const T *, const T *) -> bool { return true; }
    template<class S, class... Args>
    static auto eq(const T *lhs, const T *rhs, S pm, Args... args) -> bool
        { return (lhs->*pm) == (rhs->*pm) && eq(lhs, rhs, args...); }
};
}

SIA operator << (QIODevice &dev, const QByteArray &data) -> QIODevice&
    { dev.write(data, data.size()); return dev; }

template<int N>
SIA operator << (QIODevice &dev, const char (&str)[N]) -> QIODevice&
    { static_assert(N > 0, "!!!"); dev.write(str, N - 1); return dev; }

SIA operator << (QIODevice &dev, char ch) -> QIODevice&
    { dev.write(&ch, 1); return dev; }

#define DECL_WRITE_NUMBER(T) \
SIA operator << (QIODevice &dev, T n) -> QIODevice& { dev.write(QByteArray::number(n)); return dev; }
DECL_WRITE_NUMBER(int)
DECL_WRITE_NUMBER(qint64)
DECL_WRITE_NUMBER(uint)
DECL_WRITE_NUMBER(quint64)
DECL_WRITE_NUMBER(float)
DECL_WRITE_NUMBER(double)
#undef DECL_WRITE_NUMBER

SIA operator "" _q(const char16_t *str, size_t len) -> QString
{ return QString::fromRawData(reinterpret_cast<const QChar*>(str), len); }

SIA operator "" _a(const char *str, size_t len) -> QLatin1String
{ return QLatin1String(str, len); }

SIA operator "" _b(const char *str, size_t len) -> QByteArray
{ return QByteArray::fromRawData(str, len); }

SIA operator "" _8(const char *str, size_t len) -> QString
{ return QString::fromUtf8(str, len); }

SCIA operator "" _q(char16_t c) -> QChar { return QChar((ushort)c); }

SCIA operator "" _q(char c) -> QChar { return ushort(uchar(c)); }

template<class... Args>
using T = std::tuple<Args...>;

template<class... Args>
SCIA _T(const Args&... args) -> T<Args...>
{ return std::tuple<Args...>(args...); }

SIA _L(const char *str) -> QLatin1String
{ return QLatin1String(str); }

SIA _L(const QByteArray &b) -> QString
{ return QString::fromLatin1(b); }

SIA _N(int n, int base = 10) -> QString
{ return QString::number(n, base); }

SIA _N(quint32 n, int base = 10) -> QString
{ return QString::number(n, base); }

SIA _N(quint64 n, int base = 10) -> QString
{ return QString::number(n, base); }

SIA _N(double n, int dec = 1) -> QString
{ return QString::number(n, 'f', dec); }

SIA _N(int n, int base, int width, const QChar &c = ' '_q) -> QString
{ return u"%1"_q.arg(n, width, base, c); }

SIA _N(double n, int dec, int width, const QChar &c = ' '_q) -> QString
{ return u"%1"_q.arg(n, width, 'f', dec, c); }

template<class Conv, class Container>
SIA _ToStringList(const Container &c, Conv f) -> QStringList
{ QStringList list; for (auto &t : c) list.push_back(f(t)); return list; }

// number with sign
template<class N>
SIA _SignSymbol(N n) -> QChar { return n < 0 ? '-'_q : n > 0 ? '+'_q : u'±'_q; }

SIA _NS(int n, bool sign, int base = 10) -> QString
{ return sign ? (_SignSymbol(n) % _N(qAbs(n), base)) : _N(n, base); }

SIA _NS(double n, bool sign, int dec = 1) -> QString
{ return sign ? (_SignSymbol(n) % _N(qAbs(n), dec)) : _N(n, dec); }

template<class T>
SCIA _InRange(const T &min, const T &val, const T &max) -> bool
{ return min <= val && val <= max; }

template<class T>
SCIA _InRange0(const T &val, const T &end) -> bool
{ return T(0) <= val && val < end; }

template<class T, class S>
SCIA _IsOneOf(const T &t, const S &s) -> bool { return t == s; }

template<class T, class S, class U, class... Args>
SCIA _IsOneOf(const T &t, const S &s, const U &u, const Args&... args) -> bool
{ return t == s || _IsOneOf(t, u, args...); }

template <class T>
SIA _Change(T &the, const T &one) -> bool
{ if (the != one) {the = one; return true;} return false; }

template<class T>
SIA _C(T &t) -> const T& { return t; }

template<class T>
SIA _C(T *t) -> const T* { return t; }

SIA _MSecToTime(int ms) -> QTime { return QTime::fromMSecsSinceStartOfDay(ms); }

SIA _MSecToString(int ms, const QString &fmt = u"hh:mm:ss"_q) -> QString
{ return _MSecToTime(ms).toString(fmt); }

template<class T, typename S = T>
constexpr SIA _Max() -> S { return (S)std::numeric_limits<T>::max(); }

template<class T, typename S = T>
constexpr SIA _Min() -> S { return (S)std::numeric_limits<T>::min(); }

template<class T, typename... Args>
SIA _New(T *&t, Args... args) -> T* { return (t = new T(args...)); }

template<class T, typename... Args>
SIA _Renew(T *&t, Args... args) -> T* {delete t; return (t = new T(args...)); }

template<class T>
SIA _Delete(T *&t) -> void { if (t) { delete t; t = nullptr; } }

template<class T>
SIA _Expand(T &t, int size, double extra = 1.2) -> bool
{
    if (uint(size) < uint(t.size()))
        return false;
    t.resize(size*extra);
    return true;
}

auto _Uncompress(const QByteArray &data) -> QByteArray;
auto _JsonToString(const QJsonObject &json, bool compact = true) -> QString;
auto _JsonFromString(const QString &str) -> QJsonObject;

/**************************** file and path ***********************************/

enum class Location {
    Desktop     = QStandardPaths::DesktopLocation,
    Documents   = QStandardPaths::DocumentsLocation,
    Fonts       = QStandardPaths::FontsLocation,
    Applications= QStandardPaths::ApplicationsLocation,
    Music       = QStandardPaths::MusicLocation,
    Movies      = QStandardPaths::MoviesLocation,
    Pictures    = QStandardPaths::PicturesLocation,
    Temp        = QStandardPaths::TempLocation,
    Home        = QStandardPaths::HomeLocation,
    Data        = QStandardPaths::DataLocation,
    Cache       = QStandardPaths::GenericCacheLocation,
    Runtime     = QStandardPaths::RuntimeLocation,
    Config      = QStandardPaths::GenericConfigLocation,
    Download    = QStandardPaths::DownloadLocation
};

auto _WritablePath(Location loc, bool create = true) -> QString;

enum ExtType {
    AllExt      = 0,
    AudioExt    = 1 << 0,
    VideoExt    = 1 << 1,
    SubtitleExt = 1 << 2,
    ImageExt    = 1 << 3,
    DiscExt     = 1 << 4,
    PlaylistExt = 1 << 5,
    WritableImageExt = 1 << 6,
    WritablePlaylistExt = 1 << 7,

    MediaExt    = AudioExt | VideoExt | ImageExt
};

Q_DECLARE_FLAGS(ExtTypes, ExtType)

auto _ToAbsPath(const QString &fileName) -> QString;
auto _ToAbsFilePath(const QString &fileName) -> QString;
auto _ToNameFilter(ExtTypes exts) -> QStringList;
auto _ToFilter(ExtTypes exts) -> QString;
auto _IsSuffixOf(ExtTypes ext, const QString &suffix) -> bool;
auto _ExtList(ExtTypes ext) -> QStringList;
auto _MimeTypeForSuffix(const QString &suffix) -> QMimeType;
auto _DescriptionForSuffix(const QString &suffix) -> QString;

auto _GetOpenFiles(QWidget *parent, const QString &title, const QString &filter,
                   const QString &key = QString()) -> QStringList;

auto _GetOpenFile(QWidget *parent, const QString &title, const QString &filter,
                  const QString &key = QString()) -> QString;

auto _GetOpenFiles(QWidget *parent, const QString &title, ExtTypes exts,
                   const QString &key = QString()) -> QStringList;

auto _GetOpenFile(QWidget *parent, const QString &title, ExtTypes exts,
                  const QString &key = QString()) -> QString;

auto _GetSaveFile(QWidget *parent, const QString &title,
                  const QString &fileName, ExtTypes exts,
                  const QString &key = QString()) -> QString;

auto _GetOpenDir(QWidget *parent, const QString &title,
                 const QString &key = QString()) -> QString;

auto _SetLastOpenPath(const QString &path, const QString &k = QString()) -> void;

auto _LastOpenPath(const QString &key = QString()) -> QString;

auto _UrlFromLocalFile(const QString &file) -> QUrl;

}

using namespace Global;

Q_DECLARE_OPERATORS_FOR_FLAGS(Global::ExtTypes)

#endif // GLOBAL_HPP

