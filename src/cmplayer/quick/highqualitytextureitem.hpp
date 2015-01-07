//#ifndef HIGHQUALITYTEXTUREITEM_HPP
//#define HIGHQUALITYTEXTUREITEM_HPP

//#include "simpletextureitem.hpp"

//enum class Interpolator;            enum class Dithering;

//class HighQualityTextureItem : public SimpleTextureItem {
//    Q_OBJECT
//public:
//    HighQualityTextureItem(QQuickItem *parent = nullptr);
//    ~HighQualityTextureItem();
//    auto interpolator() const -> Interpolator;
//    auto setInterpolator(Interpolator type) -> void;
//    auto setDithering(Dithering dithering) -> void;
//    auto dithering() const -> Dithering;
//    auto type() const -> Type* override;
//    auto setOverlayTexture(const OpenGLTexture2D &overlay) -> void;
//    auto transparentTexture() const -> const OpenGLTexture2D&;
//    static auto supportsHighQualityRendering() -> bool;
//protected:
//    auto initializeGL() -> void override;
//    auto finalizeGL() -> void override;
//private:
//    ShaderIface *createShader() const override;
//    ShaderData *createData() const override;
//    auto updateData(ShaderData *data) -> void override;
//private:
//    struct Data;
//    Data *d;
//};

//#endif // HIGHQUALITYTEXTUREITEM_HPP
