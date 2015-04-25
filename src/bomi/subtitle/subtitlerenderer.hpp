#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "quick/simpletextureitem.hpp"
#include "player/streamtrack.hpp"

class SubComp;                          class Subtitle;
class RichTextDocument;
struct OsdStyle;                        class SubtitleDrawer;
enum class AutoselectMode;

class SubtitleRenderer : public SimpleTextureItem  {
    Q_OBJECT
public:
    SubtitleRenderer(QQuickItem *parent = nullptr);
    ~SubtitleRenderer();
    auto previous() const -> int;
    auto next() const -> int;
    auto current() const -> int;
    auto start(int pos) const -> int;
    auto finish(int pos) const -> int;
    auto delay() const -> int;
    auto fps() const -> double;
    auto pos() const -> double;
    auto isTopAligned() const -> bool;
    auto selection() const -> QVector<SubComp>;
    auto components() const -> QVector<const SubComp *>;
    auto addComponents(const QVector<SubComp> &components) -> void;
    auto setComponents(const QVector<SubComp> &components) -> void;
    auto componentsCount() const -> int;
    auto setPriority(const QStringList &priority) -> void;
    auto setPos(double pos) -> void;
    auto isHidden() const -> bool;
    auto setDelay(int delay) -> void;
//    auto load(const Subtitle &subtitle, bool select) -> bool;
    auto unload() -> void;
    auto select(int id) -> void;
    auto deselect(int id = -1) -> void;
    auto style() const -> const OsdStyle&;
    auto setStyle(const OsdStyle &style) -> void;
    auto text() const -> const RichTextDocument&;
    auto draw(const QRectF &rect, QRectF *put = nullptr) const -> QImage;
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
    auto setHidden(bool hidden) -> void;
    auto render(int ms) -> void;
    auto setTopAligned(bool top) -> void;
    auto setFPS(double fps) -> void;
    auto toTrackList() const -> StreamList;
    auto lastUpdatedTime() const -> int;
//    auto load(const QVector<StreamTrack> &tracks) -> void;
signals:
    void updated(int time);
    void selectionChanged();
private:
    auto initializeGL() -> void;
    auto finalizeGL() -> void;
    auto rerender() -> void;
    auto customEvent(QEvent *event) -> void override;
    auto geometryChanged(const QRectF &n, const QRectF &o) -> void override;
    auto afterUpdate() -> void;
    auto createShader() const -> ShaderIface* override;
    auto createData() const -> ShaderData* override;
    auto type() const -> Type* override { static Type type; return &type; }
    auto updateTexture(OpenGLTexture2D *texture) -> void override;
    auto updateData(ShaderData *data) -> void override;
    auto updateVertex(Vertex *vertex) -> void override;
    struct Data; Data *d;
    friend class SubtitleRendererShader;
};

#endif // SUBTITLERENDERERITEM_HPP
