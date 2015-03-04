#include "fontcombobox.hpp"
#include "misc/simplelistmodel.hpp"
#include <QFontDatabase>

static QFontDatabase::WritingSystem writingSystemFromScript(QLocale::Script script)
{
    switch (script) {
    case QLocale::ArabicScript:
        return QFontDatabase::Arabic;
    case QLocale::CyrillicScript:
        return QFontDatabase::Cyrillic;
    case QLocale::GurmukhiScript:
        return QFontDatabase::Gurmukhi;
    case QLocale::SimplifiedHanScript:
        return QFontDatabase::SimplifiedChinese;
    case QLocale::TraditionalHanScript:
        return QFontDatabase::TraditionalChinese;
    case QLocale::LatinScript:
        return QFontDatabase::Latin;
    case QLocale::ArmenianScript:
        return QFontDatabase::Armenian;
    case QLocale::BengaliScript:
        return QFontDatabase::Bengali;
    case QLocale::DevanagariScript:
        return QFontDatabase::Devanagari;
    case QLocale::GeorgianScript:
        return QFontDatabase::Georgian;
    case QLocale::GreekScript:
        return QFontDatabase::Greek;
    case QLocale::GujaratiScript:
        return QFontDatabase::Gujarati;
    case QLocale::HebrewScript:
        return QFontDatabase::Hebrew;
    case QLocale::JapaneseScript:
        return QFontDatabase::Japanese;
    case QLocale::KhmerScript:
        return QFontDatabase::Khmer;
    case QLocale::KannadaScript:
        return QFontDatabase::Kannada;
    case QLocale::KoreanScript:
        return QFontDatabase::Korean;
    case QLocale::LaoScript:
        return QFontDatabase::Lao;
    case QLocale::MalayalamScript:
        return QFontDatabase::Malayalam;
    case QLocale::MyanmarScript:
        return QFontDatabase::Myanmar;
    case QLocale::TamilScript:
        return QFontDatabase::Tamil;
    case QLocale::TeluguScript:
        return QFontDatabase::Telugu;
    case QLocale::ThaanaScript:
        return QFontDatabase::Thaana;
    case QLocale::ThaiScript:
        return QFontDatabase::Thai;
    case QLocale::TibetanScript:
        return QFontDatabase::Tibetan;
    case QLocale::SinhalaScript:
        return QFontDatabase::Sinhala;
    case QLocale::SyriacScript:
        return QFontDatabase::Syriac;
    case QLocale::OriyaScript:
        return QFontDatabase::Oriya;
    case QLocale::OghamScript:
        return QFontDatabase::Ogham;
    case QLocale::RunicScript:
        return QFontDatabase::Runic;
    case QLocale::NkoScript:
        return QFontDatabase::Nko;
    default:
        return QFontDatabase::Any;
    }
}

static QFontDatabase::WritingSystem writingSystemFromLocale()
{
    QStringList uiLanguages = QLocale::system().uiLanguages();
    QLocale::Script script;
    if (!uiLanguages.isEmpty())
        script = QLocale(uiLanguages.at(0)).script();
    else
        script = QLocale::system().script();

    return writingSystemFromScript(script);
}

static QFontDatabase::WritingSystem writingSystemForFont(QFontDatabase *db, const QFont &font, bool *hasLatin)
{
    QList<QFontDatabase::WritingSystem> writingSystems = db->writingSystems(font.family());

    // this just confuses the algorithm below. Vietnamese is Latin with lots of special chars
    writingSystems.removeOne(QFontDatabase::Vietnamese);
    *hasLatin = writingSystems.removeOne(QFontDatabase::Latin);

    if (writingSystems.isEmpty())
        return QFontDatabase::Any;

    QFontDatabase::WritingSystem system = writingSystemFromLocale();

    if (writingSystems.contains(system))
        return system;

    if (system == QFontDatabase::TraditionalChinese
            && writingSystems.contains(QFontDatabase::SimplifiedChinese)) {
        return QFontDatabase::SimplifiedChinese;
    }

    if (system == QFontDatabase::SimplifiedChinese
            && writingSystems.contains(QFontDatabase::TraditionalChinese)) {
        return QFontDatabase::TraditionalChinese;
    }

    system = writingSystems.last();

    if (!*hasLatin) {
        // we need to show something
        return system;
    }

    if (writingSystems.count() == 1 && system > QFontDatabase::Cyrillic)
        return system;

    if (writingSystems.count() <= 2 && system > QFontDatabase::Armenian && system < QFontDatabase::Vietnamese)
        return system;

    if (writingSystems.count() <= 5 && system >= QFontDatabase::SimplifiedChinese && system <= QFontDatabase::Korean)
        return system;

    return QFontDatabase::Any;
}

struct FontData {
    DECL_EQ(FontData, &T::font);
    QFont font;
    QString sample;
};

class FontFamilyModel : public SimpleListModel<FontData> {
    auto displayData(int row, int) const -> QVariant
    {
        auto &data = at(row);
        if (data.sample.isEmpty())
            return data.font.family();
        return QString(data.font.family() % " ("_a % data.sample % ")"_a);
    }
    auto fontData(int row, int) const -> QFont { return at(row).font; }

};

struct FontComboBox::Data {
    FontComboBox *p = nullptr;
    FontFamilyModel *model = nullptr;
    bool fixed = false;
    auto generateList() -> QList<FontData>
    {
        QList<FontData> list;
        QFontDatabase db;
        const auto type = fixed ? QFontDatabase::FixedFont : QFontDatabase::GeneralFont;
        const auto def = QFontDatabase::systemFont(type);
        for (auto &family : db.families()) {
            if (fixed && !db.isFixedPitch(family))
                continue;
            FontData data;
            data.font = def;
            data.font.setFamily(family);
            bool hasLatin = false;
            const auto system = writingSystemForFont(&db, data.font, &hasLatin);
            data.sample = db.writingSystemSample(system);
            list.push_back(data);
        }
        return list;
    }
};

FontComboBox::FontComboBox(QWidget *parent)
    : QComboBox(parent), d(new Data)
{
    d->p = this;
    connect(SIGNAL_VT(this, currentIndexChanged, int), this, [=] (int idx) {
        emit currentFontChanged();
        if (idx < 0)
            return;
        setFont(d->model->at(idx).font);
    });
    d->model = new FontFamilyModel;
    d->model->setList(d->generateList());
    setModel(d->model);
    setCurrentIndex(0);
}

FontComboBox::~FontComboBox()
{
    delete d->model;
    delete d;
}

auto FontComboBox::setFixedFont(bool fixed) -> void
{
    if (_Change(d->fixed, fixed)) {
        d->model->setList(d->generateList());
        setCurrentIndex(0);
    }
}

auto FontComboBox::setCurrentFont(const QFont &font) -> void
{
    const auto family = QFontInfo(font).family();
    for (int i = 0; i < d->model->size(); ++i) {
        if (QFontInfo(d->model->at(i).font).family() == family) {
            setCurrentIndex(i);
            break;
        }
    }
}

auto FontComboBox::currentFont() const -> QFont
{
    return d->model->value(currentIndex()).font;
}
