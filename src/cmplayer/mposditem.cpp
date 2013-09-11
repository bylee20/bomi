#include "mposditem.hpp"
#include "dataevent.hpp"
extern "C" {
#include <sub/sub.h>
}
#include "mposdbitmap.hpp"
#include "mposdnode.hpp"




struct MpOsdItem::Data {
	MpOsdBitmap osd;
	bool forced = true;
	bool show = false, redraw = false, dirtyGeometry = true;
	MpOsdNode *node = nullptr;
	QTimer sizeChecker;
	QSize targetSize = {0, 0}, renderSize = {0, 0}, prevSize = {0, 0};
};

MpOsdItem::MpOsdItem(QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
	setFlag(ItemHasContents, true);
	connect(&d->sizeChecker, &QTimer::timeout, [this] () {
		if (!_Change(d->prevSize, QSizeF(width(), height()).toSize())) {
			d->sizeChecker.stop();
			d->targetSize = d->prevSize;
		}
	});
	d->sizeChecker.setInterval(300);
}

MpOsdItem::~MpOsdItem() {
	delete d;
}

QSize MpOsdItem::targetSize() const {
	return d->targetSize;
}

void MpOsdItem::drawOn(sub_bitmaps *imgs) {
	d->show = true;
	if (imgs->num_parts <= 0)
		return;
//	qDebug() <<imgs->scaled;
	MpOsdBitmap osd;
	if (osd.copy(imgs, d->renderSize)) {
		postData(this, EnqueueFrame, osd);
	}
}

void MpOsdItem::drawOn(QImage &frame) {
	if (!isVisible() || !d->node)
		return;
//	QPainter painter(&frame);
//	painter.drawImage(0, 0, d->node->fbo()->toImage());
}

void MpOsdItem::present() {
	if (d->show) {
		postData(this, Show);
		d->show = false;
	} else
		postData(this, Hide);
}

void MpOsdItem::customEvent(QEvent *event) {
	QQuickItem::customEvent(event);
	switch ((int)event->type()) {
	case Show:
		setVisible(true);
		update();
		break;
	case Hide:
		setVisible(false);
		break;
	case EnqueueFrame:
		d->osd = getData<MpOsdBitmap>(event);
		d->redraw = true;
		update();
	default:
		break;
	}
}

QSGNode *MpOsdItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	Q_UNUSED(data);
	d->node = static_cast<MpRgbaOsdNode*>(old);
	if (!d->node || d->node->format() != d->osd.format()) {
		delete d->node;
		switch (d->osd.format()) {
		case MpOsdBitmap::Rgba:
		case MpOsdBitmap::RgbaPA:
			d->node = new MpRgbaOsdNode(d->osd.format());
			break;
		case MpOsdBitmap::Ass:
			d->node = new MpAssOsdNode;
			break;
		default:
			return d->node = nullptr;
		}
	}
	if (!old || d->redraw)
		d->node->draw(d->osd, boundingRect());
	if (!old || d->dirtyGeometry)
		d->node->markDirty(QSGNode::DirtyGeometry);
	d->redraw = false;
	d->dirtyGeometry = false;
	return d->node;
}

void MpOsdItem::forceUpdateTargetSize() {
	d->forced = true;
}

void MpOsdItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	QQuickItem::geometryChanged(newOne, old);
	if (d->forced) {
		d->targetSize = newOne.size().toSize();
		d->forced = false;
	}else
		d->sizeChecker.start();
	d->dirtyGeometry = true;
	d->redraw = true;
	update();

}

void MpOsdItem::setRenderSize(const QSize &size) {
	d->renderSize = size;
}
