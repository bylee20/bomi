#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "stdafx.hpp"
#include "quick/simpletextureitem.hpp"

class SubComp;                         class Subtitle;
class RichTextDocument;                class SubCompModel;
struct SubtitleStyle;                  class SubtitleDrawer;

class SubtitleRendererItem : public SimpleTextureItem  {
    Q_OBJECT
public:
    SubtitleRendererItem(QQuickItem *parent = nullptr);
    ~SubtitleRendererItem();
    auto previous() const -> int;
    auto next() const -> int;
    auto current() const -> int;
    auto start(int pos) const -> int;
    auto finish(int pos) const -> int;
    auto delay() const -> int;
    auto fps() const -> double;
    auto pos() const -> double;
    auto isTopAligned() const -> bool;
    auto models() const -> QVector<SubCompModel*>;
    auto components() const -> QList<const SubComp *>;
    auto setComponents(const QList<SubComp> &components) -> void;
    auto componentsCount() const -> int;
    auto setPriority(const QStringList &priority) -> void;
    auto setPos(double pos) -> void;
    auto isHidden() const -> bool;
    auto setDelay(int delay) -> void;
    auto load(const Subtitle &subtitle, bool select) -> bool;
    auto unload() -> void;
    auto select(int idx) -> void;
    auto deselect(int idx = -1) -> void;
    auto style() const -> const SubtitleStyle&;
    auto setStyle(const SubtitleStyle &style) -> void;
    auto text() const -> const RichTextDocument&;
    auto draw(const QRectF &rect, QRectF *put = nullptr) const -> QImage;
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
    auto setHidden(bool hidden) -> void;
    auto render(int ms) -> void;
    auto setTopAligned(bool top) -> void;
    auto setFPS(double fps) -> void;
signals:
    void modelsChanged(const QVector<SubCompModel*> &models);
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
