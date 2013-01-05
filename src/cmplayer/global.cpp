#include "global.hpp"

namespace Global {

QDialogButtonBox *makeButtonBox(QDialog *dlg) {
	QDialogButtonBox *dbb = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
	dlg->connect(dbb, SIGNAL(accepted()), SLOT(accept()));
	dlg->connect(dbb, SIGNAL(rejected()), SLOT(reject()));
	return dbb;
}

class FastAlphaBlur {
public:
	FastAlphaBlur(): radius(-1) {}
	// copied from openframeworks superfast blur and modified
	void applyTo(QImage &mask, const QColor &color, int radius) {
		if (radius < 1 || mask.isNull())
			return;
		setSize(mask.size());
		setRadius(radius);
		const int w = s.width();
		const int h = s.height();

		uchar *a = valpha.data();
		const uchar *inv = vinv.constData();
		int *min = vmin.data();
		int *max = vmax.data();

		const int xmax = mask.width()-1;
		for (int x=0; x<w; ++x) {
			min[x] = qMin(x + radius + 1, xmax);
			max[x] = qMax(x - radius, 0);
		}

		const uchar *c_bits = mask.constBits()+3;
		uchar *it = a;
		for (int y=0; y<h; ++y, c_bits += (mask.width() << 2)) {
			int sum = 0;
			for(int i=-radius; i<=radius; ++i)
				sum += c_bits[qBound(0, i, xmax) << 2];
			for (int x=0; x<w; ++x, ++it) {
				sum += c_bits[min[x] << 2];
				sum -= c_bits[max[x] << 2];
				*it = inv[sum];
			}
		}

		const int ymax = mask.height()-1;
		for (int y=0; y<h; ++y){
			min[y] = qMin(y + radius + 1, ymax)*w;
			max[y] = qMax(y - radius, 0)*w;
		}

		uchar *bits = mask.bits();
		const double r = color.redF();
		const double g = color.greenF();
		const double b = color.blueF();
		const double coef = color.alphaF();
		const uchar *c_it = a;
		for (int x=0; x<w; ++x, ++c_it){
			int sum = 0;
			int yp = -radius*w;
			for(int i=-radius; i<=radius; ++i, yp += w)
				sum += c_it[qMax(0, yp)];
			uchar *p = bits + (x << 2) - 1;
			for (int y=0; y<h; ++y, p += (xmax << 2)){
				const uchar a = inv[sum];
				if (p[4] < 255) {
					*(++p) = a*b*coef;
					*(++p) = a*g*coef;
					*(++p) = a*r*coef;
					*(++p) = a*coef;
				} else
					p += 4;
				sum += c_it[min[y]];
				sum -= c_it[max[y]];
			}
		}
	}

private:
	void setSize(const QSize &size) {
		if (size != s) {
			s = size;
			if (!s.isEmpty()) {
				vmin.resize(qMax(s.width(), s.height()));
				vmax.resize(vmin.size());
				valpha.resize(s.width()*s.height());
			}
		}
	}
	void setRadius(const int radius) {
		if (this->radius != radius) {
			this->radius = radius;
			if (radius > 0) {
				const int range = (radius << 1) + 1;
				vinv.resize(range << 8);
				for (int i=0; i<vinv.size(); ++i)
					vinv[i] = i/range;
			}
		}
	}
	QSize s;
	QVector<int> vmin, vmax;
	QVector<uchar> valpha, vinv;
	int radius;
};

}
