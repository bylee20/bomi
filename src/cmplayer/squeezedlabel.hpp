#ifndef SQUEEZEDLABEL_HPP
#define SQUEEZEDLABEL_HPP

#include <QtGui/QLabel>

// from KDE's KSqueezedTextLabel
class SqueezedLabel : public QLabel {
	Q_OBJECT
public:
	SqueezedLabel(QWidget *parent) : QLabel(parent), m_elide(Qt::ElideMiddle) {
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	}
	virtual QSize minimumSizeHint() const {
		QSize size = QLabel::minimumSizeHint();
		size.setWidth(-1);
		return size;
	}
	virtual QSize sizeHint() const {
		int maxWidth = width() * 3 / 4;
		QFontMetrics fm(fontMetrics());
		int textWidth = fm.width(m_text);
		if (textWidth > maxWidth)
			textWidth = maxWidth;
		return QSize(textWidth, QLabel::sizeHint().height());
	}
	virtual void setAlignment(Qt::Alignment alignment) {
		QString temp = m_text;
		QLabel::setAlignment(alignment);
		m_text = temp;
	}
	Qt::TextElideMode textElideMode() const {return m_elide;}
	void setTextElideMode(Qt::TextElideMode mode) {m_elide = mode; elideText();}
public slots:
	void setText(const QString &text) {m_text = text; elideText();}
protected:
	void resizeEvent(QResizeEvent *) {elideText();}
private:
	void elideText() {
		QFontMetrics fm = fontMetrics();
		int lw = size().width();
		int tw = fm.width(m_text);
		QLabel::setText(tw > lw ? fm.elidedText(m_text, m_elide, lw) : m_text);
	}
	Qt::TextElideMode m_elide;
	QString m_text;
};

#endif // SQUEEZEDLABEL_HPP
