#ifndef INTERPOLATOR_HPP
#define INTERPOLATOR_HPP

#include "openglcompat.hpp"
#include "enums.hpp"

class Interpolator {
public:
	class Texture : public OpenGLTexture1D {
	public:
		float multiplier() const { return m_mul; }
	private:
		friend class Interpolator;
		float m_mul = 2.0f;
	};
	enum Category {
		None = 0,
		Fetch16 = 1,
		Fetch36 = 2,
		Fetch64 = 3,
		Fast4 = 4,
		Fast9 = 5,
		CategoryMax
	};
	using Type = InterpolatorType;
	using Info = InterpolatorTypeInfo;
	~Interpolator();
	Type type() const;
	Category category() const;
	static Category category(Type type);
	static QByteArray shader(Category category);
	static int textures(Category category);
	static const Interpolator *get(Type type);
	QByteArray shader() const { return shader(category()); }
	int textures() const { return textures(category()); }
	void allocate(Texture &texture1, Texture &texture2) const;
private:
	Interpolator(Type type);
	friend struct Objs;
	struct Data;
	Data *d;
};

#endif // INTERPOLATOR_HPP
