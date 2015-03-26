#include "locale.hpp"
#include <unicode/locid.h>
#include "misc/log.hpp"

auto operator << (QDataStream &out, const ::Locale &l) -> QDataStream&
{
    out << l.toVariant(); return out;
}

auto operator >> (QDataStream &in, ::Locale &l) -> QDataStream&
{
    QVariant var;
    in >> var;
    l = ::Locale::fromVariant(var);
    return in;
}

DECLARE_LOG_CONTEXT(Locale)

struct Data {
    Data() {
        b2t[u"cze"_q] = u"ces"_q;
        b2t[u"baq"_q] = u"eus"_q;
        b2t[u"fre"_q] = u"fra"_q;
        b2t[u"ger"_q] = u"deu"_q;
        b2t[u"gre"_q] = u"ell"_q;
        b2t[u"arm"_q] = u"hye"_q;
        b2t[u"ice"_q] = u"isl"_q;
        b2t[u"geo"_q] = u"kat"_q;
        b2t[u"mac"_q] = u"mkd"_q;
        b2t[u"mao"_q] = u"mri"_q;
        b2t[u"may"_q] = u"msa"_q;
        b2t[u"bur"_q] = u"mya"_q;
        b2t[u"dut"_q] = u"nld"_q;
        b2t[u"per"_q] = u"fas"_q;
        b2t[u"rum"_q] = u"ron"_q;
        b2t[u"slo"_q] = u"slk"_q;
        b2t[u"alb"_q] = u"sqi"_q;
        b2t[u"tib"_q] = u"bod"_q;
        b2t[u"wel"_q] = u"cym"_q;
        b2t[u"chi"_q] = u"zho"_q;

        // custom code for opensubtitles
        b2t[u"scc"_q] = u"srp"_q;
        b2t[u"pob"_q] = u"por"_q;
        b2t[u"pb"_q]  = u"pt"_q;

        isoLanguage[u"abk"_q] = QLocale::Abkhazian;
        isoLanguage[u"aar"_q] = QLocale::Afar;
        isoLanguage[u"afr"_q] = QLocale::Afrikaans;
        isoLanguage[u"agq"_q] = QLocale::Aghem;
        isoLanguage[u"aka"_q] = QLocale::Akan;
        isoLanguage[u"akk"_q] = QLocale::Akkadian;
        isoLanguage[u"sqi"_q] = QLocale::Albanian;
        isoLanguage[u"amh"_q] = QLocale::Amharic;
        isoLanguage[u"egy"_q] = QLocale::AncientEgyptian;
        isoLanguage[u"grc"_q] = QLocale::AncientGreek;
        isoLanguage[u"ara"_q] = QLocale::Arabic;
        isoLanguage[u"arg"_q] = QLocale::Aragonese;
        isoLanguage[u"arc"_q] = QLocale::Aramaic;
        isoLanguage[u"hye"_q] = QLocale::Armenian;
        isoLanguage[u"asm"_q] = QLocale::Assamese;
        isoLanguage[u"ast"_q] = QLocale::Asturian;
        isoLanguage[u"asa"_q] = QLocale::Asu;
        isoLanguage[u"cch"_q] = QLocale::Atsam;
        isoLanguage[u"ava"_q] = QLocale::Avaric;
        isoLanguage[u"ave"_q] = QLocale::Avestan;
        isoLanguage[u"aym"_q] = QLocale::Aymara;
        isoLanguage[u"aze"_q] = QLocale::Azerbaijani;
        isoLanguage[u"ksf"_q] = QLocale::Bafia;
        isoLanguage[u"ban"_q] = QLocale::Balinese;
        isoLanguage[u"bam"_q] = QLocale::Bambara;
        isoLanguage[u"bax"_q] = QLocale::Bamun;
        isoLanguage[u"bas"_q] = QLocale::Basaa;
        isoLanguage[u"bak"_q] = QLocale::Bashkir;
        isoLanguage[u"eus"_q] = QLocale::Basque;
        isoLanguage[u"bbc"_q] = QLocale::BatakToba;
        isoLanguage[u"bel"_q] = QLocale::Belarusian;
        isoLanguage[u"bem"_q] = QLocale::Bemba;
        isoLanguage[u"bez"_q] = QLocale::Bena;
        isoLanguage[u"ben"_q] = QLocale::Bengali;
        isoLanguage[u"bih"_q] = QLocale::Bihari;
        isoLanguage[u"bis"_q] = QLocale::Bislama;
        isoLanguage[u"byn"_q] = QLocale::Blin;
        isoLanguage[u"brx"_q] = QLocale::Bodo;
        isoLanguage[u"bos"_q] = QLocale::Bosnian;
        isoLanguage[u"bre"_q] = QLocale::Breton;
        isoLanguage[u"bug"_q] = QLocale::Buginese;
        isoLanguage[u"bku"_q] = QLocale::Buhid;
        isoLanguage[u"bul"_q] = QLocale::Bulgarian;
        isoLanguage[u"mya"_q] = QLocale::Burmese;
        isoLanguage[u"xcr"_q] = QLocale::Carian;
        isoLanguage[u"cat"_q] = QLocale::Catalan;
        isoLanguage[u"tzm"_q] = QLocale::CentralMoroccoTamazight;
        isoLanguage[u"ccp"_q] = QLocale::Chakma;
        isoLanguage[u"cha"_q] = QLocale::Chamorro;
        isoLanguage[u"che"_q] = QLocale::Chechen;
        isoLanguage[u"chr"_q] = QLocale::Cherokee;
        isoLanguage[u"nya"_q] = QLocale::Chewa;
        isoLanguage[u"cgg"_q] = QLocale::Chiga;
        isoLanguage[u"zho"_q] = QLocale::Chinese;
        isoLanguage[u"chu"_q] = QLocale::Church;
        isoLanguage[u"chv"_q] = QLocale::Chuvash;
        isoLanguage[u"myz"_q] = QLocale::ClassicalMandaic;
        isoLanguage[u"ksh"_q] = QLocale::Colognian;
        isoLanguage[u"swc"_q] = QLocale::CongoSwahili;
        isoLanguage[u"cop"_q] = QLocale::Coptic;
        isoLanguage[u"cor"_q] = QLocale::Cornish;
        isoLanguage[u"cos"_q] = QLocale::Corsican;
        isoLanguage[u"cre"_q] = QLocale::Cree;
        isoLanguage[u"hrv"_q] = QLocale::Croatian;
        isoLanguage[u"ces"_q] = QLocale::Czech;
        isoLanguage[u"dan"_q] = QLocale::Danish;
        isoLanguage[u"div"_q] = QLocale::Divehi;
        isoLanguage[u"doi"_q] = QLocale::Dogri;
        isoLanguage[u"dua"_q] = QLocale::Duala;
        isoLanguage[u"nld"_q] = QLocale::Dutch;
        isoLanguage[u"dzo"_q] = QLocale::Dzongkha;
        isoLanguage[u"cjm"_q] = QLocale::EasternCham;
        isoLanguage[u"eky"_q] = QLocale::EasternKayah;
        isoLanguage[u"ebu"_q] = QLocale::Embu;
        isoLanguage[u"eng"_q] = QLocale::English;
        isoLanguage[u"epo"_q] = QLocale::Esperanto;
        isoLanguage[u"est"_q] = QLocale::Estonian;
        isoLanguage[u"ett"_q] = QLocale::Etruscan;
        isoLanguage[u"ewe"_q] = QLocale::Ewe;
        isoLanguage[u"ewo"_q] = QLocale::Ewondo;
        isoLanguage[u"fao"_q] = QLocale::Faroese;
        isoLanguage[u"fij"_q] = QLocale::Fijian;
        isoLanguage[u"fil"_q] = QLocale::Filipino;
        isoLanguage[u"fin"_q] = QLocale::Finnish;
        isoLanguage[u"fra"_q] = QLocale::French;
        isoLanguage[u"fur"_q] = QLocale::Friulian;
        isoLanguage[u"ful"_q] = QLocale::Fulah;
        isoLanguage[u"gaa"_q] = QLocale::Ga;
        isoLanguage[u"gla"_q] = QLocale::Gaelic;
        isoLanguage[u"glg"_q] = QLocale::Galician;
        isoLanguage[u"lug"_q] = QLocale::Ganda;
        isoLanguage[u"gez"_q] = QLocale::Geez;
        isoLanguage[u"kat"_q] = QLocale::Georgian;
        isoLanguage[u"deu"_q] = QLocale::German;
        isoLanguage[u"got"_q] = QLocale::Gothic;
        isoLanguage[u"ell"_q] = QLocale::Greek;
        isoLanguage[u"kal"_q] = QLocale::Greenlandic;
        isoLanguage[u"grn"_q] = QLocale::Guarani;
        isoLanguage[u"guj"_q] = QLocale::Gujarati;
        isoLanguage[u"guz"_q] = QLocale::Gusii;
        isoLanguage[u"hat"_q] = QLocale::Haitian;
        isoLanguage[u"hnn"_q] = QLocale::Hanunoo;
        isoLanguage[u"hau"_q] = QLocale::Hausa;
        isoLanguage[u"haw"_q] = QLocale::Hawaiian;
        isoLanguage[u"heb"_q] = QLocale::Hebrew;
        isoLanguage[u"her"_q] = QLocale::Herero;
        isoLanguage[u"hin"_q] = QLocale::Hindi;
        isoLanguage[u"hmo"_q] = QLocale::HiriMotu;
        isoLanguage[u"hun"_q] = QLocale::Hungarian;
        isoLanguage[u"isl"_q] = QLocale::Icelandic;
        isoLanguage[u"ibo"_q] = QLocale::Igbo;
        isoLanguage[u"ind"_q] = QLocale::Indonesian;
        isoLanguage[u"inh"_q] = QLocale::Ingush;
        isoLanguage[u"ina"_q] = QLocale::Interlingua;
        isoLanguage[u"ile"_q] = QLocale::Interlingue;
        isoLanguage[u"iku"_q] = QLocale::Inuktitut;
        isoLanguage[u"ipk"_q] = QLocale::Inupiak;
        isoLanguage[u"gle"_q] = QLocale::Irish;
        isoLanguage[u"ita"_q] = QLocale::Italian;
        isoLanguage[u"jpn"_q] = QLocale::Japanese;
        isoLanguage[u"jav"_q] = QLocale::Javanese;
        isoLanguage[u"kaj"_q] = QLocale::Jju;
        isoLanguage[u"dyo"_q] = QLocale::JolaFonyi;
        isoLanguage[u"kea"_q] = QLocale::Kabuverdianu;
        isoLanguage[u"kab"_q] = QLocale::Kabyle;
        isoLanguage[u"kkj"_q] = QLocale::Kako;
        isoLanguage[u"kln"_q] = QLocale::Kalenjin;
        isoLanguage[u"kam"_q] = QLocale::Kamba;
        isoLanguage[u"kan"_q] = QLocale::Kannada;
        isoLanguage[u"kau"_q] = QLocale::Kanuri;
        isoLanguage[u"kas"_q] = QLocale::Kashmiri;
        isoLanguage[u"kaz"_q] = QLocale::Kazakh;
        isoLanguage[u"khm"_q] = QLocale::Khmer;
        isoLanguage[u"kik"_q] = QLocale::Kikuyu;
        isoLanguage[u"kin"_q] = QLocale::Kinyarwanda;
        isoLanguage[u"kir"_q] = QLocale::Kirghiz;
        isoLanguage[u"kom"_q] = QLocale::Komi;
        isoLanguage[u"kon"_q] = QLocale::Kongo;
        isoLanguage[u"kok"_q] = QLocale::Konkani;
        isoLanguage[u"kor"_q] = QLocale::Korean;
        isoLanguage[u"jkr"_q] = QLocale::Koro;
        isoLanguage[u"ses"_q] = QLocale::KoyraboroSenni;
        isoLanguage[u"khq"_q] = QLocale::KoyraChiini;
        isoLanguage[u"kpe"_q] = QLocale::Kpelle;
        isoLanguage[u"kur"_q] = QLocale::Kurdish;
        isoLanguage[u"kua"_q] = QLocale::Kwanyama;
        isoLanguage[u"nmg"_q] = QLocale::Kwasio;
        isoLanguage[u"lag"_q] = QLocale::Langi;
        isoLanguage[u"lao"_q] = QLocale::Lao;
        isoLanguage[u"hmd"_q] = QLocale::LargeFloweryMiao;
        isoLanguage[u"lat"_q] = QLocale::Latin;
        isoLanguage[u"lav"_q] = QLocale::Latvian;
        isoLanguage[u"lep"_q] = QLocale::Lepcha;
        isoLanguage[u"lif"_q] = QLocale::Limbu;
        isoLanguage[u"lim"_q] = QLocale::Limburgish;
        isoLanguage[u"lin"_q] = QLocale::Lingala;
        isoLanguage[u"lis"_q] = QLocale::Lisu;
        isoLanguage[u"lit"_q] = QLocale::Lithuanian;
        isoLanguage[u"nds"_q] = QLocale::LowGerman;
        isoLanguage[u"khb"_q] = QLocale::Lu;
        isoLanguage[u"lub"_q] = QLocale::LubaKatanga;
        isoLanguage[u"luo"_q] = QLocale::Luo;
        isoLanguage[u"ltz"_q] = QLocale::Luxembourgish;
        isoLanguage[u"luy"_q] = QLocale::Luyia;
        isoLanguage[u"xlc"_q] = QLocale::Lycian;
        isoLanguage[u"xld"_q] = QLocale::Lydian;
        isoLanguage[u"mkd"_q] = QLocale::Macedonian;
        isoLanguage[u"jmc"_q] = QLocale::Machame;
        isoLanguage[u"mgh"_q] = QLocale::MakhuwaMeetto;
        isoLanguage[u"kde"_q] = QLocale::Makonde;
        isoLanguage[u"mlg"_q] = QLocale::Malagasy;
        isoLanguage[u"msa"_q] = QLocale::Malay;
        isoLanguage[u"mal"_q] = QLocale::Malayalam;
        isoLanguage[u"mlt"_q] = QLocale::Maltese;
        isoLanguage[u"mnk"_q] = QLocale::Mandingo;
        isoLanguage[u"mni"_q] = QLocale::Manipuri;
        isoLanguage[u"glv"_q] = QLocale::Manx;
        isoLanguage[u"mri"_q] = QLocale::Maori;
        isoLanguage[u"mar"_q] = QLocale::Marathi;
        isoLanguage[u"mah"_q] = QLocale::Marshallese;
        isoLanguage[u"mas"_q] = QLocale::Masai;
        isoLanguage[u"xmr"_q] = QLocale::Meroitic;
        isoLanguage[u"mer"_q] = QLocale::Meru;
        isoLanguage[u"mon"_q] = QLocale::Mongolian;
        isoLanguage[u"mfe"_q] = QLocale::Morisyen;
        isoLanguage[u"mua"_q] = QLocale::Mundang;
        isoLanguage[u"naq"_q] = QLocale::Nama;
        isoLanguage[u"nau"_q] = QLocale::NauruLanguage;
        isoLanguage[u"nav"_q] = QLocale::Navaho;
        isoLanguage[u"ndo"_q] = QLocale::Ndonga;
        isoLanguage[u"nep"_q] = QLocale::Nepali;
        isoLanguage[u"nnh"_q] = QLocale::Ngiemboon;
        isoLanguage[u"jgo"_q] = QLocale::Ngomba;
        isoLanguage[u"sme"_q] = QLocale::NorthernSami;
        isoLanguage[u"nso"_q] = QLocale::NorthernSotho;
        isoLanguage[u"nod"_q] = QLocale::NorthernThai;
        isoLanguage[u"nde"_q] = QLocale::NorthNdebele;
        isoLanguage[u"nor"_q] = QLocale::Norwegian;
        isoLanguage[u"nno"_q] = QLocale::NorwegianNynorsk;
        isoLanguage[u"nus"_q] = QLocale::Nuer;
        isoLanguage[u"nya"_q] = QLocale::Nyanja;
        isoLanguage[u"nyn"_q] = QLocale::Nyankole;
        isoLanguage[u"oci"_q] = QLocale::Occitan;
        isoLanguage[u"oji"_q] = QLocale::Ojibwa;
        isoLanguage[u"sga"_q] = QLocale::OldIrish;
        isoLanguage[u"non"_q] = QLocale::OldNorse;
        isoLanguage[u"peo"_q] = QLocale::OldPersian;
        isoLanguage[u"otk"_q] = QLocale::OldTurkish;
        isoLanguage[u"ori"_q] = QLocale::Oriya;
        isoLanguage[u"orm"_q] = QLocale::Oromo;
        isoLanguage[u"oss"_q] = QLocale::Ossetic;
        isoLanguage[u"pal"_q] = QLocale::Pahlavi;
        isoLanguage[u"pli"_q] = QLocale::Pali;
        isoLanguage[u"xpr"_q] = QLocale::Parthian;
        isoLanguage[u"pus"_q] = QLocale::Pashto;
        isoLanguage[u"fas"_q] = QLocale::Persian;
        isoLanguage[u"phn"_q] = QLocale::Phoenician;
        isoLanguage[u"pol"_q] = QLocale::Polish;
        isoLanguage[u"por"_q] = QLocale::Portuguese;
        isoLanguage[u"pra"_q] = QLocale::PrakritLanguage;
        isoLanguage[u"pan"_q] = QLocale::Punjabi;
        isoLanguage[u"que"_q] = QLocale::Quechua;
        isoLanguage[u"rej"_q] = QLocale::Rejang;
        isoLanguage[u"ron"_q] = QLocale::Romanian;
        isoLanguage[u"roh"_q] = QLocale::Romansh;
        isoLanguage[u"rof"_q] = QLocale::Rombo;
        isoLanguage[u"run"_q] = QLocale::Rundi;
        isoLanguage[u"rus"_q] = QLocale::Russian;
        isoLanguage[u"rwk"_q] = QLocale::Rwa;
        isoLanguage[u"xsa"_q] = QLocale::Sabaean;
        isoLanguage[u"ssy"_q] = QLocale::Saho;
        isoLanguage[u"sah"_q] = QLocale::Sakha;
        isoLanguage[u"sam"_q] = QLocale::Samaritan;
        isoLanguage[u"saq"_q] = QLocale::Samburu;
        isoLanguage[u"smo"_q] = QLocale::Samoan;
        isoLanguage[u"sag"_q] = QLocale::Sango;
        isoLanguage[u"sbp"_q] = QLocale::Sangu;
        isoLanguage[u"san"_q] = QLocale::Sanskrit;
        isoLanguage[u"sat"_q] = QLocale::Santali;
        isoLanguage[u"srd"_q] = QLocale::Sardinian;
        isoLanguage[u"saz"_q] = QLocale::Saurashtra;
        isoLanguage[u"she"_q] = QLocale::Sena;
        isoLanguage[u"srp"_q] = QLocale::Serbian;
        isoLanguage[u"ksb"_q] = QLocale::Shambala;
        isoLanguage[u"sna"_q] = QLocale::Shona;
        isoLanguage[u"iii"_q] = QLocale::SichuanYi;
        isoLanguage[u"sid"_q] = QLocale::Sidamo;
        isoLanguage[u"snd"_q] = QLocale::Sindhi;
        isoLanguage[u"sin"_q] = QLocale::Sinhala;
        isoLanguage[u"slk"_q] = QLocale::Slovak;
        isoLanguage[u"slv"_q] = QLocale::Slovenian;
        isoLanguage[u"xog"_q] = QLocale::Soga;
        isoLanguage[u"som"_q] = QLocale::Somali;
        isoLanguage[u"srb"_q] = QLocale::Sora;
        isoLanguage[u"sot"_q] = QLocale::SouthernSotho;
        isoLanguage[u"nbl"_q] = QLocale::SouthNdebele;
        isoLanguage[u"spa"_q] = QLocale::Spanish;
        isoLanguage[u"sun"_q] = QLocale::Sundanese;
        isoLanguage[u"swa"_q] = QLocale::Swahili;
        isoLanguage[u"ssw"_q] = QLocale::Swati;
        isoLanguage[u"swe"_q] = QLocale::Swedish;
        isoLanguage[u"gsw"_q] = QLocale::SwissGerman;
        isoLanguage[u"syl"_q] = QLocale::Sylheti;
        isoLanguage[u"syc"_q] = QLocale::Syriac;
        isoLanguage[u"shi"_q] = QLocale::Tachelhit;
        isoLanguage[u"tgl"_q] = QLocale::Tagalog;
        isoLanguage[u"tbw"_q] = QLocale::Tagbanwa;
        isoLanguage[u"tah"_q] = QLocale::Tahitian;
        isoLanguage[u"blt"_q] = QLocale::TaiDam;
        isoLanguage[u"tdd"_q] = QLocale::TaiNua;
        isoLanguage[u"dav"_q] = QLocale::Taita;
        isoLanguage[u"tgk"_q] = QLocale::Tajik;
        isoLanguage[u"tam"_q] = QLocale::Tamil;
        isoLanguage[u"trv"_q] = QLocale::Taroko;
        isoLanguage[u"twq"_q] = QLocale::Tasawaq;
        isoLanguage[u"tat"_q] = QLocale::Tatar;
        isoLanguage[u"tel"_q] = QLocale::Telugu;
        isoLanguage[u"teo"_q] = QLocale::Teso;
        isoLanguage[u"tha"_q] = QLocale::Thai;
        isoLanguage[u"bod"_q] = QLocale::Tibetan;
        isoLanguage[u"tig"_q] = QLocale::Tigre;
        isoLanguage[u"tir"_q] = QLocale::Tigrinya;
        isoLanguage[u"ton"_q] = QLocale::Tongan;
        isoLanguage[u"tso"_q] = QLocale::Tsonga;
        isoLanguage[u"tsn"_q] = QLocale::Tswana;
        isoLanguage[u"tur"_q] = QLocale::Turkish;
        isoLanguage[u"tuk"_q] = QLocale::Turkmen;
        isoLanguage[u"twi"_q] = QLocale::Twi;
        isoLanguage[u"kcg"_q] = QLocale::Tyap;
        isoLanguage[u"uga"_q] = QLocale::Ugaritic;
        isoLanguage[u"uig"_q] = QLocale::Uighur;
        isoLanguage[u"ukr"_q] = QLocale::Ukrainian;
        isoLanguage[u"urd"_q] = QLocale::Urdu;
        isoLanguage[u"uzb"_q] = QLocale::Uzbek;
        isoLanguage[u"vai"_q] = QLocale::Vai;
        isoLanguage[u"ven"_q] = QLocale::Venda;
        isoLanguage[u"vie"_q] = QLocale::Vietnamese;
        isoLanguage[u"vol"_q] = QLocale::Volapuk;
        isoLanguage[u"vun"_q] = QLocale::Vunjo;
        isoLanguage[u"wal"_q] = QLocale::Walamo;
        isoLanguage[u"wln"_q] = QLocale::Walloon;
        isoLanguage[u"wae"_q] = QLocale::Walser;
        isoLanguage[u"cym"_q] = QLocale::Welsh;
        isoLanguage[u"fry"_q] = QLocale::WesternFrisian;
        isoLanguage[u"wol"_q] = QLocale::Wolof;
        isoLanguage[u"xho"_q] = QLocale::Xhosa;
        isoLanguage[u"yav"_q] = QLocale::Yangben;
        isoLanguage[u"yid"_q] = QLocale::Yiddish;
        isoLanguage[u"yor"_q] = QLocale::Yoruba;
        isoLanguage[u"dje"_q] = QLocale::Zarma;
        isoLanguage[u"zha"_q] = QLocale::Zhuang;
        isoLanguage[u"zul"_q] = QLocale::Zulu;
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
        isoLanguage[u"bss"_q] = QLocale::Akoose;
        isoLanguage[u"lkt"_q] = QLocale::Lakota;
        isoLanguage[u"zgh"_q] = QLocale::StandardMoroccanTamazight;
#endif
    }
    QLocale icu;
    ::Locale native = ::Locale::system();
    QHash<QString, QString> b2t;
    QMap<QString, QString> isoName;
    QMap<QString, QLocale::Language> isoLanguage;
};

static auto data() -> Data& { static Data d; return d; }

::Locale::Locale(const Locale &rhs)
{
    if (rhs.m_locale)
        m_locale = new QLocale(*rhs.m_locale);
}

::Locale::Locale(Locale &&rhs)
{
    std::swap(m_locale, rhs.m_locale);
}

auto ::Locale::operator = (const Locale &rhs) -> Locale&
{
    if (this != &rhs) {
        _Delete(m_locale);
        if (rhs.m_locale)
            m_locale = new QLocale(*rhs.m_locale);
    }
    return *this;
}

auto ::Locale::operator = (Locale &&rhs) -> Locale&
{
    if (this != &rhs)
        std::swap(m_locale, rhs.m_locale);
    return *this;
}

auto ::Locale::native() -> Locale
{
    return data().native;
}

auto ::Locale::setNative(const Locale &l) -> void
{
    data().native = l;
    data().icu = QLocale(l.name());
}

auto ::Locale::isoToNativeName(const QString &_iso) -> QString
{
    if (_iso.size() > 3)
        return QString();
    QString iso = _iso.toLower();
    auto it = data().b2t.constFind(iso);
    if (it != data().b2t.cend())
        iso = *it;
    auto &name = data().isoName[iso];
    if (name.isEmpty()) {
        const auto lang = data().isoLanguage.value(iso, QLocale::C);
        if (lang == QLocale::C) {
            _Error("Cannot find locale for %%", iso);
            name = iso;
        } else
            name = QLocale(lang).nativeLanguageName();
    }
    return name;
}

auto ::Locale::nativeName() const -> QString
{
    if (!m_locale)
        return QString();
    if (m_locale->language() == QLocale::C)
        return u"C"_q;
    return m_locale->nativeLanguageName();
}

// dummy for pref
auto ::Locale::toJson() const -> QJsonObject
{
    return QJsonObject();
}

auto ::Locale::setFromJson(const QJsonObject &json) -> bool
{
    Q_UNUSED(json);
    return true;
}
