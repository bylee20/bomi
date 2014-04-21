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
    InterpolatorType interpolator() const;
    void setInterpolator(InterpolatorType type);
    void setDithering(Dithering dithering);
    Dithering dithering() const;
    Type *type() const override;
protected:
    void initializeGL() override;
    void finalizeGL() override;
private:
    ShaderIface *createShader() const override;
    ShaderData *createData() const override;
    void updateData(ShaderData *data) override;
private:
    struct Data;
    Data *d;
};

#endif // HIGHQUALITYTEXTUREITEM_HPP
