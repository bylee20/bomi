#include "richtextblock.hpp"

static QTextOption makeRubyOption() {
	QTextOption opt;
	opt.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
	opt.setWrapMode(QTextOption::NoWrap);
	return opt;
}

static const QTextOption rubyOption = makeRubyOption();

RichTextBlock::Layout RichTextBlock::toLayout(const QTextOption &option, const QTextCharFormat &format) const {
	Layout layout;
	makeLayout(layout.block, layout.rubies, option, format);
	return layout;
}

void RichTextBlock::makeLayout(QTextLayout *&block, QList<QTextLayout*> &rubies, const QTextOption &option, const QTextCharFormat &format) const {
	block = new QTextLayout;
	rubies.reserve(this->rubies.size());
	for (int i=0; i<this->rubies.size(); ++i)
		rubies[i] = new QTextLayout;
	fillLayout(block, rubies, option, format);
}

void RichTextBlock::fillLayout(QTextLayout *block, const QList<QTextLayout*> &rubies, const QTextOption &option, const QTextCharFormat &format) const {
	auto mergeFormat = [&format](QTextCharFormat &aFormat, const Style &style) {
		aFormat = format;
		for (auto it = style.begin(); it != style.end(); ++it)
			aFormat.setProperty(it.key(), it.value());
	};

	block->setTextOption(option);
	block->setText(text);
	QList<QTextLayout::FormatRange> ranges;
	for (int i=0; i<formats.size(); ++i) {
		ranges << QTextLayout::FormatRange();
		QTextLayout::FormatRange &range = ranges.last();
		range.start = formats[i].begin;
		range.length = formats[i].end - formats[i].begin;
		mergeFormat(range.format, formats[i].style);
	}
	block->setAdditionalFormats(ranges);

	Q_ASSERT(rubies.size() == this->rubies.size());
	for (int i=0; i<rubies.size(); ++i) {
		rubies[i]->setTextOption(rubyOption);
		rubies[i]->setText(this->rubies[i].rt);
		QList<QTextLayout::FormatRange> ranges;
		ranges << QTextLayout::FormatRange();
		ranges.last().start = 0;
		ranges.last().length = this->rubies[i].rt.length();
		mergeFormat(ranges.last().format, this->rubies[i].rb_style);
		ranges.last().format.setProperty(QTextFormat::FontPixelSize, (int)(ranges.last().format.intProperty(QTextFormat::FontPixelSize)*0.45 + 0.5));
		rubies[i]->setAdditionalFormats(ranges);
	}
}


static const double MaxLeading = 1e10;

QRectF RichTextBlock::doLayout(bool top, QTextLayout *block, const QList<QTextLayout*> &rubies, int maxWidth, double &leading, QPointF &pos) const {
	Q_ASSERT(rubies.size() == this->rubies.size());

	QRectF rect(pos, QSizeF(0, 0));
	double width = 0;

	bool first = true;

	block->beginLayout();

	int rt_idx = 0;
	forever {
		QTextLine line = block->createLine();
		if (!line.isValid())
			break;
		line.setLineWidth(maxWidth);
		line.setPosition(pos);
		double rt_height = 0.0;
		while (rt_idx < rubies.size()) {
			const Ruby &ruby = this->rubies[rt_idx];
			if (!(line.textStart() <= ruby.rb_begin && ruby.rb_end <= line.textStart() + line.textLength()))
				break;
			const int left = line.cursorToX(ruby.rb_begin);
			const int right = line.cursorToX(ruby.rb_end);
			if (left < right) {
				QTextLayout *layout = rubies[rt_idx];
				layout->beginLayout();
				QTextLine line_rt = layout->createLine();
				if (line_rt.isValid()) {
					line_rt.setLineWidth(right - left);
					line_rt.setPosition(QPointF(left, pos.y()));
					if (rt_height < line_rt.height())
						rt_height = line_rt.height();
				}
				layout->endLayout();
			}
			++rt_idx;
		}

		if (leading > MaxLeading)
			leading = line.leading();
		if (top && first) {
			pos.ry() += rt_height;
			first = false;
		} else
			pos.ry() += qMax(rt_height, leading);
		line.setPosition(pos);
		pos.ry() += line.height();
		if (line.naturalTextWidth() > width)
			width = line.naturalTextWidth();
	}
	block->endLayout();
	rect.setSize(QSizeF(width, pos.y()));
	return rect;
}

