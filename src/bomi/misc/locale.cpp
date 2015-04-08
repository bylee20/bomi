#include "locale.hpp"
#include "misc/log.hpp"
#include "configure.hpp"

static constexpr int s_maxLang = 315;

auto operator << (QDataStream &out, const Locale &l) -> QDataStream&
{
    out << l.toVariant(); return out;
}

auto operator >> (QDataStream &in, Locale &l) -> QDataStream&
{
    QVariant var;
    in >> var;
    l = Locale::fromVariant(var);
    return in;
}

DECLARE_LOG_CONTEXT(Locale)

struct Data {
    Data()
    {
        // ISO639-3 B to T
        aliases[u"cze"_q] = u"ces"_q;
        aliases[u"baq"_q] = u"eus"_q;
        aliases[u"fre"_q] = u"fra"_q;
        aliases[u"ger"_q] = u"deu"_q;
        aliases[u"gre"_q] = u"ell"_q;
        aliases[u"arm"_q] = u"hye"_q;
        aliases[u"ice"_q] = u"isl"_q;
        aliases[u"geo"_q] = u"kat"_q;
        aliases[u"mac"_q] = u"mkd"_q;
        aliases[u"mao"_q] = u"mri"_q;
        aliases[u"may"_q] = u"msa"_q;
        aliases[u"bur"_q] = u"mya"_q;
        aliases[u"dut"_q] = u"nld"_q;
        aliases[u"per"_q] = u"fas"_q;
        aliases[u"rum"_q] = u"ron"_q;
        aliases[u"slo"_q] = u"slk"_q;
        aliases[u"alb"_q] = u"sqi"_q;
        aliases[u"tib"_q] = u"bod"_q;
        aliases[u"wel"_q] = u"cym"_q;
        aliases[u"chi"_q] = u"zho"_q;

        // custom code for opensubtitles
        aliases[u"scc"_q] = u"srp"_q;
        aliases[u"pob"_q] = u"por"_q;
        aliases[u"pb"_q]  = u"por"_q;

        // duplicated languages
        aliases[u"tgl"_q] = u"fil"_q;
        aliases[u"twi"_q] = u"aka"_q;

        // ISO639-1 to ISO639-3
        aliases[u"aa"_q] = u"aar"_q;
        aliases[u"ab"_q] = u"abk"_q;
        aliases[u"af"_q] = u"afr"_q;
        aliases[u"ak"_q] = u"aka"_q;
        aliases[u"sq"_q] = u"sqi"_q;
        aliases[u"am"_q] = u"amh"_q;
        aliases[u"ar"_q] = u"ara"_q;
        aliases[u"an"_q] = u"arg"_q;
        aliases[u"hy"_q] = u"hye"_q;
        aliases[u"as"_q] = u"asm"_q;
        aliases[u"av"_q] = u"ava"_q;
        aliases[u"ae"_q] = u"ave"_q;
        aliases[u"ay"_q] = u"aym"_q;
        aliases[u"az"_q] = u"aze"_q;
        aliases[u"ba"_q] = u"bak"_q;
        aliases[u"bm"_q] = u"bam"_q;
        aliases[u"eu"_q] = u"eus"_q;
        aliases[u"be"_q] = u"bel"_q;
        aliases[u"bn"_q] = u"ben"_q;
        aliases[u"bh"_q] = u"bih"_q;
        aliases[u"bi"_q] = u"bis"_q;
        aliases[u"bs"_q] = u"bos"_q;
        aliases[u"br"_q] = u"bre"_q;
        aliases[u"bg"_q] = u"bul"_q;
        aliases[u"my"_q] = u"mya"_q;
        aliases[u"ca"_q] = u"cat"_q;
        aliases[u"ch"_q] = u"cha"_q;
        aliases[u"ce"_q] = u"che"_q;
        aliases[u"zh"_q] = u"zho"_q;
        aliases[u"cu"_q] = u"chu"_q;
        aliases[u"cv"_q] = u"chv"_q;
        aliases[u"kw"_q] = u"cor"_q;
        aliases[u"co"_q] = u"cos"_q;
        aliases[u"cr"_q] = u"cre"_q;
        aliases[u"cs"_q] = u"ces"_q;
        aliases[u"da"_q] = u"dan"_q;
        aliases[u"dv"_q] = u"div"_q;
        aliases[u"nl"_q] = u"nld"_q;
        aliases[u"dz"_q] = u"dzo"_q;
        aliases[u"en"_q] = u"eng"_q;
        aliases[u"eo"_q] = u"epo"_q;
        aliases[u"et"_q] = u"est"_q;
        aliases[u"ee"_q] = u"ewe"_q;
        aliases[u"fo"_q] = u"fao"_q;
        aliases[u"fj"_q] = u"fij"_q;
        aliases[u"fi"_q] = u"fin"_q;
        aliases[u"fr"_q] = u"fra"_q;
        aliases[u"fy"_q] = u"fry"_q;
        aliases[u"ff"_q] = u"ful"_q;
        aliases[u"ka"_q] = u"kat"_q;
        aliases[u"de"_q] = u"deu"_q;
        aliases[u"gd"_q] = u"gla"_q;
        aliases[u"ga"_q] = u"gle"_q;
        aliases[u"gl"_q] = u"glg"_q;
        aliases[u"gv"_q] = u"glv"_q;
        aliases[u"el"_q] = u"ell"_q;
        aliases[u"gn"_q] = u"grn"_q;
        aliases[u"gu"_q] = u"guj"_q;
        aliases[u"ht"_q] = u"hat"_q;
        aliases[u"ha"_q] = u"hau"_q;
        aliases[u"he"_q] = u"heb"_q;
        aliases[u"hz"_q] = u"her"_q;
        aliases[u"hi"_q] = u"hin"_q;
        aliases[u"ho"_q] = u"hmo"_q;
        aliases[u"hr"_q] = u"hrv"_q;
        aliases[u"hu"_q] = u"hun"_q;
        aliases[u"ig"_q] = u"ibo"_q;
        aliases[u"is"_q] = u"isl"_q;
        aliases[u"io"_q] = u"ido"_q;
        aliases[u"ii"_q] = u"iii"_q;
        aliases[u"iu"_q] = u"iku"_q;
        aliases[u"ie"_q] = u"ile"_q;
        aliases[u"ia"_q] = u"ina"_q;
        aliases[u"id"_q] = u"ind"_q;
        aliases[u"ik"_q] = u"ipk"_q;
        aliases[u"it"_q] = u"ita"_q;
        aliases[u"jv"_q] = u"jav"_q;
        aliases[u"ja"_q] = u"jpn"_q;
        aliases[u"kl"_q] = u"kal"_q;
        aliases[u"kn"_q] = u"kan"_q;
        aliases[u"ks"_q] = u"kas"_q;
        aliases[u"kr"_q] = u"kau"_q;
        aliases[u"kk"_q] = u"kaz"_q;
        aliases[u"km"_q] = u"khm"_q;
        aliases[u"ki"_q] = u"kik"_q;
        aliases[u"rw"_q] = u"kin"_q;
        aliases[u"ky"_q] = u"kir"_q;
        aliases[u"kv"_q] = u"kom"_q;
        aliases[u"kg"_q] = u"kon"_q;
        aliases[u"ko"_q] = u"kor"_q;
        aliases[u"kj"_q] = u"kua"_q;
        aliases[u"ku"_q] = u"kur"_q;
        aliases[u"lo"_q] = u"lao"_q;
        aliases[u"la"_q] = u"lat"_q;
        aliases[u"lv"_q] = u"lav"_q;
        aliases[u"li"_q] = u"lim"_q;
        aliases[u"ln"_q] = u"lin"_q;
        aliases[u"lt"_q] = u"lit"_q;
        aliases[u"lb"_q] = u"ltz"_q;
        aliases[u"lu"_q] = u"lub"_q;
        aliases[u"lg"_q] = u"lug"_q;
        aliases[u"mk"_q] = u"mkd"_q;
        aliases[u"mh"_q] = u"mah"_q;
        aliases[u"ml"_q] = u"mal"_q;
        aliases[u"mi"_q] = u"mri"_q;
        aliases[u"mr"_q] = u"mar"_q;
        aliases[u"ms"_q] = u"msa"_q;
        aliases[u"mg"_q] = u"mlg"_q;
        aliases[u"mt"_q] = u"mlt"_q;
        aliases[u"mn"_q] = u"mon"_q;
        aliases[u"na"_q] = u"nau"_q;
        aliases[u"nv"_q] = u"nav"_q;
        aliases[u"nr"_q] = u"nbl"_q;
        aliases[u"nd"_q] = u"nde"_q;
        aliases[u"ng"_q] = u"ndo"_q;
        aliases[u"ne"_q] = u"nep"_q;
        aliases[u"nn"_q] = u"nno"_q;
        aliases[u"nb"_q] = u"nob"_q;
        aliases[u"no"_q] = u"nor"_q;
        aliases[u"ny"_q] = u"nya"_q;
        aliases[u"oc"_q] = u"oci"_q;
        aliases[u"oj"_q] = u"oji"_q;
        aliases[u"or"_q] = u"ori"_q;
        aliases[u"om"_q] = u"orm"_q;
        aliases[u"os"_q] = u"oss"_q;
        aliases[u"pa"_q] = u"pan"_q;
        aliases[u"fa"_q] = u"fas"_q;
        aliases[u"pi"_q] = u"pli"_q;
        aliases[u"pl"_q] = u"pol"_q;
        aliases[u"pt"_q] = u"por"_q;
        aliases[u"ps"_q] = u"pus"_q;
        aliases[u"qu"_q] = u"que"_q;
        aliases[u"rm"_q] = u"roh"_q;
        aliases[u"ro"_q] = u"ron"_q;
        aliases[u"rn"_q] = u"run"_q;
        aliases[u"ru"_q] = u"rus"_q;
        aliases[u"sg"_q] = u"sag"_q;
        aliases[u"sa"_q] = u"san"_q;
        aliases[u"si"_q] = u"sin"_q;
        aliases[u"sk"_q] = u"slk"_q;
        aliases[u"sl"_q] = u"slv"_q;
        aliases[u"se"_q] = u"sme"_q;
        aliases[u"sm"_q] = u"smo"_q;
        aliases[u"sn"_q] = u"sna"_q;
        aliases[u"sd"_q] = u"snd"_q;
        aliases[u"so"_q] = u"som"_q;
        aliases[u"st"_q] = u"sot"_q;
        aliases[u"es"_q] = u"spa"_q;
        aliases[u"sc"_q] = u"srd"_q;
        aliases[u"sr"_q] = u"srp"_q;
        aliases[u"ss"_q] = u"ssw"_q;
        aliases[u"su"_q] = u"sun"_q;
        aliases[u"sw"_q] = u"swa"_q;
        aliases[u"sv"_q] = u"swe"_q;
        aliases[u"ty"_q] = u"tah"_q;
        aliases[u"ta"_q] = u"tam"_q;
        aliases[u"tt"_q] = u"tat"_q;
        aliases[u"te"_q] = u"tel"_q;
        aliases[u"tg"_q] = u"tgk"_q;
        aliases[u"tl"_q] = u"tgl"_q;
        aliases[u"th"_q] = u"tha"_q;
        aliases[u"bo"_q] = u"bod"_q;
        aliases[u"ti"_q] = u"tir"_q;
        aliases[u"to"_q] = u"ton"_q;
        aliases[u"tn"_q] = u"tsn"_q;
        aliases[u"ts"_q] = u"tso"_q;
        aliases[u"tk"_q] = u"tuk"_q;
        aliases[u"tr"_q] = u"tur"_q;
        aliases[u"tw"_q] = u"twi"_q;
        aliases[u"ug"_q] = u"uig"_q;
        aliases[u"uk"_q] = u"ukr"_q;
        aliases[u"ur"_q] = u"urd"_q;
        aliases[u"uz"_q] = u"uzb"_q;
        aliases[u"ve"_q] = u"ven"_q;
        aliases[u"vi"_q] = u"vie"_q;
        aliases[u"vo"_q] = u"vol"_q;
        aliases[u"cy"_q] = u"cym"_q;
        aliases[u"wa"_q] = u"wln"_q;
        aliases[u"wo"_q] = u"wol"_q;
        aliases[u"xh"_q] = u"xho"_q;
        aliases[u"yi"_q] = u"yid"_q;
        aliases[u"yo"_q] = u"yor"_q;
        aliases[u"za"_q] = u"zha"_q;
        aliases[u"zu"_q] = u"zul"_q;
    }
    Locale native = Locale::system();
    QHash<QString, QString> aliases;
    QMap<QString, QString> isoName;
    auto iso(QLocale::Language lang) const -> QJsonObject
    {
        QFile file(u":/locale-map.json"_q);
        if (!file.open(QFile::ReadOnly))
            return QJsonObject();
        QJsonParseError e;
        auto doc = QJsonDocument::fromJson(file.readAll(), &e);
        if (e.error)
            return QJsonObject();
        return doc.array().at(lang).toObject();
    }
};

static auto data() -> Data& { static Data d; return d; }

#ifdef BOMI_IMPORT_ICU

struct IcuData {
    IcuData() { }
    IcuData(const QString &fileName)
    {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly))
            return;
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QRegEx rx(uR"(^\s*([a-zA-Z0-9_]+)\{$)"_q);
        QRegEx rxLang(uR"#(^\s*([a-zA-Z0-9_]+)\{"(.*)"\}$)#"_q);
        bool inLang = false, inScript = false;
        QSet<QString> langs;
        while (!in.atEnd()) {
            auto line = in.readLine().trimmed();
            if (locale.isEmpty()) {
                auto m = rx.match(line);
                if (m.hasMatch())
                    locale = m.captured(1);
                continue;
            }
            if (!inLang && !inScript) {
                auto m = rx.match(line);
                if (!m.hasMatch())
                    continue;
                if (m.captured(1) == "Languages"_a)
                    inLang = true;
                else if (m.captured(1) == "Scripts"_a)
                    inScript = true;
                continue;
            }
            Q_ASSERT(!(inLang && inScript));
            if (line == "}"_a) {
                inScript = inLang = false;
                continue;
            }
            auto m = rxLang.match(line);
            Q_ASSERT(m.hasMatch());
            if (langs.contains(m.captured(2)))
                continue;
            langs.insert(names[m.captured(1)] = m.captured(2));
        }
    }
    QString locale;
    QMap<QString, QString> names;
};

auto Locale::importIcu() -> void
{
    const QString folder = QDir::homePath() % "/icu/source/data/lang/"_a;

    QMap<QLocale::Language, QString> iso;

    iso[QLocale::Abkhazian] = u"abk"_q;
    iso[QLocale::Afar] = u"aar"_q;
    iso[QLocale::Afrikaans] = u"afr"_q;
    iso[QLocale::Aghem] = u"agq"_q;
    iso[QLocale::Akan] = u"aka"_q;
    iso[QLocale::Akkadian] = u"akk"_q;
    iso[QLocale::Albanian] = u"sqi"_q;
    iso[QLocale::Amharic] = u"amh"_q;
    iso[QLocale::AncientEgyptian] = u"egy"_q;
    iso[QLocale::AncientGreek] = u"grc"_q;
    iso[QLocale::Arabic] = u"ara"_q;
    iso[QLocale::Aragonese] = u"arg"_q;
    iso[QLocale::Aramaic] = u"arc"_q;
    iso[QLocale::Armenian] = u"hye"_q;
    iso[QLocale::Assamese] = u"asm"_q;
    iso[QLocale::Asturian] = u"ast"_q;
    iso[QLocale::Asu] = u"asa"_q;
    iso[QLocale::Atsam] = u"cch"_q;
    iso[QLocale::Avaric] = u"ava"_q;
    iso[QLocale::Avestan] = u"ave"_q;
    iso[QLocale::Aymara] = u"aym"_q;
    iso[QLocale::Azerbaijani] = u"aze"_q;
    iso[QLocale::Bafia] = u"ksf"_q;
    iso[QLocale::Balinese] = u"ban"_q;
    iso[QLocale::Bambara] = u"bam"_q;
    iso[QLocale::Bamun] = u"bax"_q;
    iso[QLocale::Basaa] = u"bas"_q;
    iso[QLocale::Bashkir] = u"bak"_q;
    iso[QLocale::Basque] = u"eus"_q;
    iso[QLocale::BatakToba] = u"bbc"_q;
    iso[QLocale::Belarusian] = u"bel"_q;
    iso[QLocale::Bemba] = u"bem"_q;
    iso[QLocale::Bena] = u"bez"_q;
    iso[QLocale::Bengali] = u"ben"_q;
    iso[QLocale::Bihari] = u"bih"_q;
    iso[QLocale::Bislama] = u"bis"_q;
    iso[QLocale::Blin] = u"byn"_q;
    iso[QLocale::Bodo] = u"brx"_q;
    iso[QLocale::Bosnian] = u"bos"_q;
    iso[QLocale::Breton] = u"bre"_q;
    iso[QLocale::Buginese] = u"bug"_q;
    iso[QLocale::Buhid] = u"bku"_q;
    iso[QLocale::Bulgarian] = u"bul"_q;
    iso[QLocale::Burmese] = u"mya"_q;
    iso[QLocale::Carian] = u"xcr"_q;
    iso[QLocale::Catalan] = u"cat"_q;
    iso[QLocale::CentralMoroccoTamazight] = u"tzm"_q;
    iso[QLocale::Chakma] = u"ccp"_q;
    iso[QLocale::Chamorro] = u"cha"_q;
    iso[QLocale::Chechen] = u"che"_q;
    iso[QLocale::Cherokee] = u"chr"_q;
    iso[QLocale::Chewa] = u"nya"_q;
    iso[QLocale::Chiga] = u"cgg"_q;
    iso[QLocale::Chinese] = u"zho"_q;
    iso[QLocale::Church] = u"chu"_q;
    iso[QLocale::Chuvash] = u"chv"_q;
    iso[QLocale::ClassicalMandaic] = u"myz"_q;
    iso[QLocale::Colognian] = u"ksh"_q;
    iso[QLocale::CongoSwahili] = u"swc"_q;
    iso[QLocale::Coptic] = u"cop"_q;
    iso[QLocale::Cornish] = u"cor"_q;
    iso[QLocale::Corsican] = u"cos"_q;
    iso[QLocale::Cree] = u"cre"_q;
    iso[QLocale::Croatian] = u"hrv"_q;
    iso[QLocale::Czech] = u"ces"_q;
    iso[QLocale::Danish] = u"dan"_q;
    iso[QLocale::Divehi] = u"div"_q;
    iso[QLocale::Dogri] = u"doi"_q;
    iso[QLocale::Duala] = u"dua"_q;
    iso[QLocale::Dutch] = u"nld"_q;
    iso[QLocale::Dzongkha] = u"dzo"_q;
    iso[QLocale::EasternCham] = u"cjm"_q;
    iso[QLocale::EasternKayah] = u"eky"_q;
    iso[QLocale::Embu] = u"ebu"_q;
    iso[QLocale::English] = u"eng"_q;
    iso[QLocale::Esperanto] = u"epo"_q;
    iso[QLocale::Estonian] = u"est"_q;
    iso[QLocale::Etruscan] = u"ett"_q;
    iso[QLocale::Ewe] = u"ewe"_q;
    iso[QLocale::Ewondo] = u"ewo"_q;
    iso[QLocale::Faroese] = u"fao"_q;
    iso[QLocale::Fijian] = u"fij"_q;
    iso[QLocale::Filipino] = u"fil"_q;
    iso[QLocale::Finnish] = u"fin"_q;
    iso[QLocale::French] = u"fra"_q;
    iso[QLocale::Friulian] = u"fur"_q;
    iso[QLocale::Fulah] = u"ful"_q;
    iso[QLocale::Ga] = u"gaa"_q;
    iso[QLocale::Gaelic] = u"gla"_q;
    iso[QLocale::Galician] = u"glg"_q;
    iso[QLocale::Ganda] = u"lug"_q;
    iso[QLocale::Geez] = u"gez"_q;
    iso[QLocale::Georgian] = u"kat"_q;
    iso[QLocale::German] = u"deu"_q;
    iso[QLocale::Gothic] = u"got"_q;
    iso[QLocale::Greek] = u"ell"_q;
    iso[QLocale::Greenlandic] = u"kal"_q;
    iso[QLocale::Guarani] = u"grn"_q;
    iso[QLocale::Gujarati] = u"guj"_q;
    iso[QLocale::Gusii] = u"guz"_q;
    iso[QLocale::Haitian] = u"hat"_q;
    iso[QLocale::Hanunoo] = u"hnn"_q;
    iso[QLocale::Hausa] = u"hau"_q;
    iso[QLocale::Hawaiian] = u"haw"_q;
    iso[QLocale::Hebrew] = u"heb"_q;
    iso[QLocale::Herero] = u"her"_q;
    iso[QLocale::Hindi] = u"hin"_q;
    iso[QLocale::HiriMotu] = u"hmo"_q;
    iso[QLocale::Hungarian] = u"hun"_q;
    iso[QLocale::Icelandic] = u"isl"_q;
    iso[QLocale::Igbo] = u"ibo"_q;
    iso[QLocale::Indonesian] = u"ind"_q;
    iso[QLocale::Ingush] = u"inh"_q;
    iso[QLocale::Interlingua] = u"ina"_q;
    iso[QLocale::Interlingue] = u"ile"_q;
    iso[QLocale::Inuktitut] = u"iku"_q;
    iso[QLocale::Inupiak] = u"ipk"_q;
    iso[QLocale::Irish] = u"gle"_q;
    iso[QLocale::Italian] = u"ita"_q;
    iso[QLocale::Japanese] = u"jpn"_q;
    iso[QLocale::Javanese] = u"jav"_q;
    iso[QLocale::Jju] = u"kaj"_q;
    iso[QLocale::JolaFonyi] = u"dyo"_q;
    iso[QLocale::Kabuverdianu] = u"kea"_q;
    iso[QLocale::Kabyle] = u"kab"_q;
    iso[QLocale::Kako] = u"kkj"_q;
    iso[QLocale::Kalenjin] = u"kln"_q;
    iso[QLocale::Kamba] = u"kam"_q;
    iso[QLocale::Kannada] = u"kan"_q;
    iso[QLocale::Kanuri] = u"kau"_q;
    iso[QLocale::Kashmiri] = u"kas"_q;
    iso[QLocale::Kazakh] = u"kaz"_q;
    iso[QLocale::Khmer] = u"khm"_q;
    iso[QLocale::Kikuyu] = u"kik"_q;
    iso[QLocale::Kinyarwanda] = u"kin"_q;
    iso[QLocale::Kirghiz] = u"kir"_q;
    iso[QLocale::Komi] = u"kom"_q;
    iso[QLocale::Kongo] = u"kon"_q;
    iso[QLocale::Konkani] = u"kok"_q;
    iso[QLocale::Korean] = u"kor"_q;
    iso[QLocale::Koro] = u"jkr"_q;
    iso[QLocale::KoyraboroSenni] = u"ses"_q;
    iso[QLocale::KoyraChiini] = u"khq"_q;
    iso[QLocale::Kpelle] = u"kpe"_q;
    iso[QLocale::Kurdish] = u"kur"_q;
    iso[QLocale::Kwanyama] = u"kua"_q;
    iso[QLocale::Kwasio] = u"nmg"_q;
    iso[QLocale::Langi] = u"lag"_q;
    iso[QLocale::Lao] = u"lao"_q;
    iso[QLocale::LargeFloweryMiao] = u"hmd"_q;
    iso[QLocale::Latin] = u"lat"_q;
    iso[QLocale::Latvian] = u"lav"_q;
    iso[QLocale::Lepcha] = u"lep"_q;
    iso[QLocale::Limbu] = u"lif"_q;
    iso[QLocale::Limburgish] = u"lim"_q;
    iso[QLocale::Lingala] = u"lin"_q;
    iso[QLocale::Lisu] = u"lis"_q;
    iso[QLocale::Lithuanian] = u"lit"_q;
    iso[QLocale::LowGerman] = u"nds"_q;
    iso[QLocale::Lu] = u"khb"_q;
    iso[QLocale::LubaKatanga] = u"lub"_q;
    iso[QLocale::Luo] = u"luo"_q;
    iso[QLocale::Luxembourgish] = u"ltz"_q;
    iso[QLocale::Luyia] = u"luy"_q;
    iso[QLocale::Lycian] = u"xlc"_q;
    iso[QLocale::Lydian] = u"xld"_q;
    iso[QLocale::Macedonian] = u"mkd"_q;
    iso[QLocale::Machame] = u"jmc"_q;
    iso[QLocale::MakhuwaMeetto] = u"mgh"_q;
    iso[QLocale::Makonde] = u"kde"_q;
    iso[QLocale::Malagasy] = u"mlg"_q;
    iso[QLocale::Malay] = u"msa"_q;
    iso[QLocale::Malayalam] = u"mal"_q;
    iso[QLocale::Maltese] = u"mlt"_q;
    iso[QLocale::Mandingo] = u"mnk"_q;
    iso[QLocale::Manipuri] = u"mni"_q;
    iso[QLocale::Manx] = u"glv"_q;
    iso[QLocale::Maori] = u"mri"_q;
    iso[QLocale::Marathi] = u"mar"_q;
    iso[QLocale::Marshallese] = u"mah"_q;
    iso[QLocale::Masai] = u"mas"_q;
    iso[QLocale::Meroitic] = u"xmr"_q;
    iso[QLocale::Meru] = u"mer"_q;
    iso[QLocale::Mongolian] = u"mon"_q;
    iso[QLocale::Morisyen] = u"mfe"_q;
    iso[QLocale::Mundang] = u"mua"_q;
    iso[QLocale::Nama] = u"naq"_q;
    iso[QLocale::NauruLanguage] = u"nau"_q;
    iso[QLocale::Navaho] = u"nav"_q;
    iso[QLocale::Ndonga] = u"ndo"_q;
    iso[QLocale::Nepali] = u"nep"_q;
    iso[QLocale::Ngiemboon] = u"nnh"_q;
    iso[QLocale::Ngomba] = u"jgo"_q;
    iso[QLocale::NorthernSami] = u"sme"_q;
    iso[QLocale::NorthernSotho] = u"nso"_q;
    iso[QLocale::NorthernThai] = u"nod"_q;
    iso[QLocale::NorthNdebele] = u"nde"_q;
    iso[QLocale::Norwegian] = u"nor"_q;
    iso[QLocale::NorwegianNynorsk] = u"nno"_q;
    iso[QLocale::Nuer] = u"nus"_q;
    iso[QLocale::Nyanja] = u"nya"_q;
    iso[QLocale::Nyankole] = u"nyn"_q;
    iso[QLocale::Occitan] = u"oci"_q;
    iso[QLocale::Ojibwa] = u"oji"_q;
    iso[QLocale::OldIrish] = u"sga"_q;
    iso[QLocale::OldNorse] = u"non"_q;
    iso[QLocale::OldPersian] = u"peo"_q;
    iso[QLocale::OldTurkish] = u"otk"_q;
    iso[QLocale::Oriya] = u"ori"_q;
    iso[QLocale::Oromo] = u"orm"_q;
    iso[QLocale::Ossetic] = u"oss"_q;
    iso[QLocale::Pahlavi] = u"pal"_q;
    iso[QLocale::Pali] = u"pli"_q;
    iso[QLocale::Parthian] = u"xpr"_q;
    iso[QLocale::Pashto] = u"pus"_q;
    iso[QLocale::Persian] = u"fas"_q;
    iso[QLocale::Phoenician] = u"phn"_q;
    iso[QLocale::Polish] = u"pol"_q;
    iso[QLocale::Portuguese] = u"por"_q;
    iso[QLocale::PrakritLanguage] = u"pra"_q;
    iso[QLocale::Punjabi] = u"pan"_q;
    iso[QLocale::Quechua] = u"que"_q;
    iso[QLocale::Rejang] = u"rej"_q;
    iso[QLocale::Romanian] = u"ron"_q;
    iso[QLocale::Romansh] = u"roh"_q;
    iso[QLocale::Rombo] = u"rof"_q;
    iso[QLocale::Rundi] = u"run"_q;
    iso[QLocale::Russian] = u"rus"_q;
    iso[QLocale::Rwa] = u"rwk"_q;
    iso[QLocale::Sabaean] = u"xsa"_q;
    iso[QLocale::Saho] = u"ssy"_q;
    iso[QLocale::Sakha] = u"sah"_q;
    iso[QLocale::Samaritan] = u"sam"_q;
    iso[QLocale::Samburu] = u"saq"_q;
    iso[QLocale::Samoan] = u"smo"_q;
    iso[QLocale::Sango] = u"sag"_q;
    iso[QLocale::Sangu] = u"sbp"_q;
    iso[QLocale::Sanskrit] = u"san"_q;
    iso[QLocale::Santali] = u"sat"_q;
    iso[QLocale::Sardinian] = u"srd"_q;
    iso[QLocale::Saurashtra] = u"saz"_q;
    iso[QLocale::Sena] = u"she"_q;
    iso[QLocale::Serbian] = u"srp"_q;
    iso[QLocale::Shambala] = u"ksb"_q;
    iso[QLocale::Shona] = u"sna"_q;
    iso[QLocale::SichuanYi] = u"iii"_q;
    iso[QLocale::Sidamo] = u"sid"_q;
    iso[QLocale::Sindhi] = u"snd"_q;
    iso[QLocale::Sinhala] = u"sin"_q;
    iso[QLocale::Slovak] = u"slk"_q;
    iso[QLocale::Slovenian] = u"slv"_q;
    iso[QLocale::Soga] = u"xog"_q;
    iso[QLocale::Somali] = u"som"_q;
    iso[QLocale::Sora] = u"srb"_q;
    iso[QLocale::SouthernSotho] = u"sot"_q;
    iso[QLocale::SouthNdebele] = u"nbl"_q;
    iso[QLocale::Spanish] = u"spa"_q;
    iso[QLocale::Sundanese] = u"sun"_q;
    iso[QLocale::Swahili] = u"swa"_q;
    iso[QLocale::Swati] = u"ssw"_q;
    iso[QLocale::Swedish] = u"swe"_q;
    iso[QLocale::SwissGerman] = u"gsw"_q;
    iso[QLocale::Sylheti] = u"syl"_q;
    iso[QLocale::Syriac] = u"syc"_q;
    iso[QLocale::Tachelhit] = u"shi"_q;
    iso[QLocale::Tagbanwa] = u"tbw"_q;
    iso[QLocale::Tahitian] = u"tah"_q;
    iso[QLocale::TaiDam] = u"blt"_q;
    iso[QLocale::TaiNua] = u"tdd"_q;
    iso[QLocale::Taita] = u"dav"_q;
    iso[QLocale::Tajik] = u"tgk"_q;
    iso[QLocale::Tamil] = u"tam"_q;
    iso[QLocale::Taroko] = u"trv"_q;
    iso[QLocale::Tasawaq] = u"twq"_q;
    iso[QLocale::Tatar] = u"tat"_q;
    iso[QLocale::Telugu] = u"tel"_q;
    iso[QLocale::Teso] = u"teo"_q;
    iso[QLocale::Thai] = u"tha"_q;
    iso[QLocale::Tibetan] = u"bod"_q;
    iso[QLocale::Tigre] = u"tig"_q;
    iso[QLocale::Tigrinya] = u"tir"_q;
    iso[QLocale::Tongan] = u"ton"_q;
    iso[QLocale::Tsonga] = u"tso"_q;
    iso[QLocale::Tswana] = u"tsn"_q;
    iso[QLocale::Turkish] = u"tur"_q;
    iso[QLocale::Turkmen] = u"tuk"_q;
    iso[QLocale::Tyap] = u"kcg"_q;
    iso[QLocale::Ugaritic] = u"uga"_q;
    iso[QLocale::Uighur] = u"uig"_q;
    iso[QLocale::Ukrainian] = u"ukr"_q;
    iso[QLocale::Urdu] = u"urd"_q;
    iso[QLocale::Uzbek] = u"uzb"_q;
    iso[QLocale::Vai] = u"vai"_q;
    iso[QLocale::Venda] = u"ven"_q;
    iso[QLocale::Vietnamese] = u"vie"_q;
    iso[QLocale::Volapuk] = u"vol"_q;
    iso[QLocale::Vunjo] = u"vun"_q;
    iso[QLocale::Walamo] = u"wal"_q;
    iso[QLocale::Walloon] = u"wln"_q;
    iso[QLocale::Walser] = u"wae"_q;
    iso[QLocale::Welsh] = u"cym"_q;
    iso[QLocale::WesternFrisian] = u"fry"_q;
    iso[QLocale::Wolof] = u"wol"_q;
    iso[QLocale::Xhosa] = u"xho"_q;
    iso[QLocale::Yangben] = u"yav"_q;
    iso[QLocale::Yiddish] = u"yid"_q;
    iso[QLocale::Yoruba] = u"yor"_q;
    iso[QLocale::Zarma] = u"dje"_q;
    iso[QLocale::Zhuang] = u"zha"_q;
    iso[QLocale::Zulu] = u"zul"_q;
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    iso[QLocale::Akoose] = u"bss"_q;
    iso[QLocale::Lakota] = u"lkt"_q;
    iso[QLocale::StandardMoroccanTamazight] = u"zgh"_q;
#endif

    for (auto it = iso.begin(); it != iso.end(); ++it) {
        Q_ASSERT(it.key() < s_maxLang);
    }

    QMap<QString, QString> iso2icu;

    iso2icu[u"chu"_q] = u"cu"_q;
    iso2icu[u"gla"_q] = u"gd"_q;
    iso2icu[u"ipk"_q] = u"ik"_q;
    iso2icu[u"kal"_q] = u"kl"_q;
    iso2icu[u"kir"_q] = u"ky"_q;
    iso2icu[u"kua"_q] = u"kj"_q;
    iso2icu[u"lub"_q] = u"lu"_q;
    iso2icu[u"nav"_q] = u"nv"_q;
    iso2icu[u"nno"_q] = u"nn"_q;
    iso2icu[u"nor"_q] = u"nb"_q;
    iso2icu[u"uig"_q] = u"ug"_q;
    iso2icu[u"vol"_q] = u"vo"_q;

    IcuData eng(folder % "en.txt"_a);
    for (auto it = iso.begin(); it != iso.end(); ++it) {
        if (iso2icu.contains(*it)) {
            Q_ASSERT(eng.names.contains(iso2icu[*it]));
            continue;
        }
        if (eng.names.contains(*it)) {
            iso2icu[*it] = *it;
        } else {
            const auto lang = QLocale::languageToString(it.key());
            for (auto iit = eng.names.begin(); iit != eng.names.end(); ++iit) {
                if (iit.value() == lang) {
                    iso2icu[*it] = iit.key();
                    break;
                }
            }
        }
    }

    QJsonArray json;
    for (int i = 0; i < s_maxLang; ++i) {
        json.push_back(QJsonObject());
    }
    for (auto it = iso.begin(); it != iso.end(); ++it) {
        const auto icu = iso2icu[*it];
        if (icu.isEmpty()) {
            qDebug() << "pass" << QLocale::languageToString(it.key());
            continue;
        }
        const auto fileName = QString(folder % icu % ".txt"_a);
        IcuData data;
        if (!QFileInfo(fileName).exists()) {
            qDebug() << "no:" << icu << QLocale::languageToString(it.key());
        } else {
            data = IcuData(fileName);
            Q_ASSERT(!data.names.isEmpty());
        }
        QJsonObject map;
        map.insert(u"iso639"_q, *it);
        for (auto iit = iso.begin(); iit != iso.end(); ++iit) {
            const auto name = data.names.value(iso2icu.value(*iit));
            if (!name.isEmpty())
                map.insert(*iit, name);
        }
        json[it.key()] = map;
    }

    QFile file(QDir::homePath() % "/dev/bomi/src/bomi/locale-map.json"_a);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        Q_ASSERT(false);
    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented).replace("    "_b, "\t"_b));
}
#endif

Locale::Locale(const Locale &rhs)
{
    if (rhs.m_locale)
        m_locale = new QLocale(*rhs.m_locale);
}

Locale::Locale(Locale &&rhs)
{
    std::swap(m_locale, rhs.m_locale);
}

auto Locale::operator = (const Locale &rhs) -> Locale&
{
    if (this != &rhs) {
        _Delete(m_locale);
        if (rhs.m_locale)
            m_locale = new QLocale(*rhs.m_locale);
    }
    return *this;
}

auto Locale::operator = (Locale &&rhs) -> Locale&
{
    if (this != &rhs)
        std::swap(m_locale, rhs.m_locale);
    return *this;
}

auto Locale::native() -> Locale
{
    return data().native;
}

auto Locale::setNative(const Locale &l) -> void
{
    data().native = l;
}

auto Locale::isoToNativeName(const QString &_iso) -> QString
{
    if (_iso.size() > 3)
        return QString();
    QString iso = _iso.toLower();
    auto it = data().aliases.constFind(iso);
    if (it != data().aliases.cend())
        iso = *it;
    auto &name = data().isoName[iso];
    if (name.isEmpty()) {
        const auto map = data().iso(data().native.language());
        const auto lang = map[iso].toString();
        if (lang.isEmpty()) {
            _Error("Cannot find locale for %%", iso);
            name = iso;
        } else
            name = lang;
    }
    return name;
}

auto Locale::nativeName() const -> QString
{
    if (!m_locale)
        return QString();
    if (m_locale->language() == QLocale::C)
        return u"C"_q;
    return m_locale->nativeLanguageName();
}

// dummy for pref
auto Locale::toJson() const -> QJsonObject
{
    return QJsonObject();
}

auto Locale::setFromJson(const QJsonObject &json) -> bool
{
    Q_UNUSED(json);
    return true;
}
