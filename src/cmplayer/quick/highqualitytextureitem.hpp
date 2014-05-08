#ifndef HIGHQUALITYTEXTUREITEM_HPP
#define HIGHQUALITYTEXTUREITEM_HPP

#include "stdafx.hpp"
#include "simpletextureitem.hpp"

class TextureRendererShader;

enum class InterpolatorType;
enum class Dithering;

class HighQualityTextureItem : public SimpleTextureItem {
    Q_OBJECT
public:
    HighQualityTextureItem(QQuickItem *parent = nullptr);
    ~HighQualityTextureItem();
    auto interpolator() const -> InterpolatorType;
    auto setInterpolator(InterpolatorType type) -> void;
    auto setDithering(Dithering dithering) -> void;
    auto dithering() const -> Dithering;
    Type *type() const override;
protected:
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
private:
    ShaderIface *createShader() const override;
    ShaderData *createData() const override;
    auto updateData(ShaderData *data) -> void override;
private:
    struct Data;
    Data *d;
};

#endif // HIGHQUALITYTEXTUREITEM_HPP
