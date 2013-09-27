//#ifndef VIDEOTEXTURESHADER_HPP
//#define VIDEOTEXTURESHADER_HPP

//#include "videorendereritem.hpp"
//#include "colorproperty.hpp"
//#include "videoframe.hpp"
//#include "deintinfo.hpp"
//#include "openglcompat.hpp"

//struct VideoTexture : public OpenGLTexture { int plane = 0; };

//struct VideoTextureUploader {
//	VideoTextureUploader(const DeintInfo &deint = DeintInfo()): m_deint(deint) {}
//	virtual ~VideoTextureUploader() {}
//	virtual void initialize(const VideoTexture &tex) { tex.allocate(); }
//	virtual void upload(const VideoTexture &tex, const VideoFrame &frame) { tex.upload2D(frame.data(tex.plane)); }
//	virtual QImage toImage(const VideoFrame &frame) const { return frame.toImage(); }
//	virtual mp_csp colorspace(const VideoFormat &format) const { return format.colorspace(); }
//	virtual mp_csp_levels colorrange(const VideoFormat &format) const { return format.range(); }
//	const DeintInfo &deint() const { return m_deint; }
//private:
//	DeintInfo m_deint;
//};

//class VideoTextureShader {
//public:
//	static constexpr int KernelEffects = VideoRendererItem::KernelEffects;
//	static VideoTextureShader *create(const VideoFormat &format
//		, const ColorProperty &color = ColorProperty()
//		, InterpolatorType interpolator = InterpolatorType::Bilinear
//		, const DeintInfo &deint = DeintInfo(), int effects = 0);
//	VideoTextureShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D);
//	virtual ~VideoTextureShader();
//	void initialize(GLuint *textures);
//	const VideoFormat &format() const {return m_format;}
//	const char *fragment() const;
//	const char *vertex() const;
//	const char *const *attributes() const;
//	void getCoords(double &x1, double &y1, double &x2, double &y2) const { m_rect.getCoords(&x1, &y1, &x2, &y2); }
//	void link(QOpenGLShaderProgram *program);
//	void render(QOpenGLShaderProgram *program, const Kernel3x3 &kernel);
//	void upload(const VideoFrame &frame);
//	QImage toImage(const VideoFrame &frame) const {return m_uploader->toImage(frame);}
//	void setColor(const ColorProperty &color) { m_color = color; updateMatrix(); }
//	bool setEffects(int effects);
//	void upload(const QImage &image);
//	bool hasKernelEffects() const {return m_effects & KernelEffects;}
//	bool isCompatibleWith(const VideoFormat &format) const;
//protected:
//	void setTexel(const QByteArray &codes) {m_texel = codes;}
//	QPointF &sc(int idx) {return *m_strideCorrection[idx];}
//	QPointF sc(int idx) const {return *m_strideCorrection[idx];}
//	void addTexInfo(int plane, int width, int height, GLenum format) {
//		addTexInfo(plane, width, height, OpenGLCompat::get().textureFormat(format));
//	}
//	void addTexInfo(int plane, int width, int height, const OpenGLTextureFormat &format) {
//		VideoTexture texture;
//		texture.plane = plane;
//		texture.target = m_target;
//		texture.width = width;
//		texture.height = height;
//		texture.format = format;
//		m_textures.append(texture);
//	}
//private:
//	QByteArray header() const;
//	void updateMatrix();
//	void setUploader(VideoTextureUploader *uploader) { if (m_uploader) delete m_uploader; m_uploader = uploader; }
//	VideoFormat m_format;
//	GLenum m_target = GL_TEXTURE_2D;
//	mutable QByteArray m_header;
//	// texel correction for second and third planar. occasionally, the stride does not match exactly, e.g, for I420, stride[1]*2 != stride[0]
//	QPointF m_sc1 = {1.0, 1.0}, m_sc2 = {1.0, 1.0}, m_sc3 = {1.0, 1.0};
//	QPointF *m_strideCorrection[3] = {&m_sc1, &m_sc2, &m_sc3};
//	int loc_kern_d, loc_kern_c, loc_kern_n, loc_top_field, loc_deint, loc_p1, loc_p2, loc_p3;
//	int loc_sc2, loc_sc3, loc_cubic_lut, loc_sub_vec, loc_add_vec, loc_mul_mat;//, loc_conv_lut;
//	QRectF m_rect = {0, 0, 1, 1};
//	QVector<VideoTexture> m_textures;
//	VideoTextureUploader *m_uploader = nullptr;
//	QMatrix3x3 m_mul_mat; QVector3D m_add_vec, m_sub_vec;
//	QByteArray m_texel = "vec3 texel(const in vec2 coord) {return texture1(coord).xyz;}";
//	ColorProperty m_color;
//	int m_effects = 0, m_field = 0;
//	bool m_top = false, m_updateLut = true;
//	DeintInfo m_deint;
//	OpenGLTexture m_cubicLutTexture; // bicubic look-up table
////	OpenGLTexture m_convLutTexture; // color conversion filter look-up table
//	std::array<GLushort, 128*4> m_cubicLut;
//	mutable QByteArray m_fragCode, m_vertexCode;
//	InterpolatorType m_interpolator;
//};

//#endif // TEXTURESHADER_HPP
