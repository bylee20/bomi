#include "richtextdocument.hpp"

RichTextDocument::RichTextDocument() {
	setChanged(false);
}

RichTextDocument::RichTextDocument(const QString &text) {
	setText(text);
}

RichTextDocument::RichTextDocument(const RichTextDocument &rhs)
: m_blocks(rhs.blocks()), m_option(rhs.m_option), m_format(rhs.m_format) {
	setChanged(!m_blocks.isEmpty());
}

RichTextDocument::~RichTextDocument() {
	freeLayouts();
}

void RichTextDocument::freeLayouts() {
	for (auto &layout : m_layouts) {
		qDeleteAll(layout->rubies);
		_Delete(layout);
	}
	m_layouts.clear();
}

RichTextDocument &RichTextDocument::operator = (const RichTextDocument &rhs) {
	if (this != &rhs) {
		m_blocks = rhs.blocks();
		m_option = rhs.m_option;
		m_format = rhs.m_format;
		setChanged(true);
	}
	return *this;
}

RichTextDocument &RichTextDocument::operator = (const QList<RichTextBlock> &rhs) {
	if (&m_blocks != &rhs) {
		m_blocks = rhs;
		setChanged(true);
	}
	return *this;
}

RichTextDocument &RichTextDocument::operator += (const RichTextDocument &rhs) {
	return operator += (rhs.blocks());
}

RichTextDocument &RichTextDocument::operator += (const QList<RichTextBlock> &rhs) {
	if (!rhs.isEmpty()) {
		setChanged(true);
		m_blocks += rhs;
	}
	return *this;
}

void RichTextDocument::setAlignment(Qt::Alignment alignment) {
	if (m_option.alignment() != alignment) {
		m_option.setAlignment(alignment);
		m_dirty = m_optionChanged = true;
	}
}

void RichTextDocument::setWrapMode(QTextOption::WrapMode wrapMode) {
	if (m_option.wrapMode() != wrapMode) {
		m_option.setWrapMode(wrapMode);
		m_dirty = m_optionChanged = true;
	}
}

void RichTextDocument::setFormat(QTextFormat::Property property, const QVariant &data) {
	if (m_format.property(property) != data) {
		m_format.setProperty(property, data);
		m_dirty = m_formatChanged = true;
	}
}

void RichTextDocument::setLeading(double newLine, double paragraph) {
	if (!qFuzzyCompare(newLine, m_lineLeading) || !qFuzzyCompare(paragraph, m_paragraphLeading)) {
		m_lineLeading = newLine;
		m_paragraphLeading = paragraph;
		m_dirty = true;
	}
}

void RichTextDocument::setFontPixelSize(int px) {
	if (m_format.intProperty(QTextFormat::FontPixelSize) != px) {
		m_format.setProperty(QTextFormat::FontPixelSize, px);
		m_dirty = m_pxChanged = true;
	}
}

void RichTextDocument::setTextOutline(const QPen &pen) {
	if (m_format.penProperty(QTextFormat::TextOutline) != pen) {
		m_format.setProperty(QTextFormat::TextOutline, pen);
		m_dirty = m_pxChanged = true;
	}
}

void RichTextDocument::setText(const QString &text) {
	m_blocks.clear();
	RichTextBlockParser parser(text.midRef(0));
	while (!parser.atEnd())
		m_blocks += parser.paragraph();
	setChanged(true);
}


static QTextOption makeRubyOption() {
	QTextOption opt;
	opt.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
	opt.setWrapMode(QTextOption::NoWrap);
	return opt;
}

static const QTextOption rubyOption = makeRubyOption();

void RichTextDocument::doLayout(double maxWidth) {
	if (!m_dirty)
		return;
	double width = -1;
	const int px = m_format.intProperty(QTextFormat::FontPixelSize);
	QPointF pos(0, 0);
	for (int i=0; i<m_layouts.size(); ++i) {
		auto &block = m_layouts[i]->block;
		auto &rubies = m_layouts[i]->rubies;

		block.beginLayout();
		int rt_idx = 0;
		for (;;) {
			QTextLine line = block.createLine();
			if (!line.isValid())
				break;
			line.setLineWidth(maxWidth);
			line.setPosition(pos);
			double rt_height = 0.0;
			while (rt_idx < rubies.size()) {
				const auto &ruby = m_blocks[i].rubies[rt_idx];
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

			const bool empty = m_blocks[i].text.isEmpty();
			if (!empty) {
				if (width < 0) {// first line
					pos.ry() += rt_height;
				} else {
					const auto leading = m_blocks[i].paragraph ? m_paragraphLeading : m_lineLeading;
					pos.ry() += qMax(rt_height, leading);
				}
				line.setPosition(pos);
				pos.ry() += line.height();
				if (line.naturalTextWidth() > width)
					width = line.naturalTextWidth();
			} else if (width >= 0)
				pos.ry() += px;
		}
		block.endLayout();
	}
	m_natural = QRectF(0.0, 0.0, width, pos.ry());
	m_dirty = false;
}

void RichTextDocument::updateLayoutInfo() {
	if (m_blockChanged) {
		freeLayouts();
		m_layouts.resize(m_blocks.size());
		m_formatChanged = m_optionChanged = true;
		for (int i=0; i<m_blocks.size(); ++i) {
			auto layout = m_layouts[i] = new Layout;
			layout->block.setText(m_blocks[i].text);
			layout->rubies.resize(m_blocks[i].rubies.size());
			for (int j=0; j<layout->rubies.size(); ++j) {
				layout->rubies[j] = new QTextLayout;
				layout->rubies[j]->setText(m_blocks[i].rubies[j].rt);
				layout->rubies[j]->setTextOption(rubyOption);
			}

		}
	}
	if (m_optionChanged) {
		for (auto layout : m_layouts)
			layout->block.setTextOption(m_option);
	}
	if (!m_formatChanged && m_pxChanged) {
		for (int i=0; i<m_blocks.size(); ++i) {
			const auto &block = m_blocks[i];
			auto layout = m_layouts[i];
			auto ranges = layout->block.additionalFormats();
			for (int j=0; j<ranges.size(); ++j) {
				if (!block.formats[j].style.contains(QTextFormat::FontPixelSize)) {
					ranges[j].format.setProperty(QTextFormat::FontPixelSize, m_format.property(QTextFormat::FontPixelSize));
					ranges[j].format.setProperty(QTextFormat::TextOutline, m_format.property(QTextFormat::TextOutline));
				}
			}
			layout->block.setAdditionalFormats(ranges);

			int idx = 0;
			for (auto ruby : layout->rubies) {
				auto ranges = ruby->additionalFormats();
				if (!block.rubies[idx++].rb_style.contains(RichTextBlock::FontPixelSize)) {
					const int px = m_format.intProperty(QTextFormat::FontPixelSize)*0.45 + 0.5;
					ranges.last().format.setProperty(QTextFormat::FontPixelSize, px);
					ranges.last().format.setProperty(QTextFormat::TextOutline, m_format.property(QTextFormat::TextOutline));
				}
				ruby->setAdditionalFormats(ranges);
			}
		}
	} else if (m_formatChanged) {
		auto mergeFormat = [this](QTextCharFormat &format, const RichTextBlock::Style &style) {
			format = m_format;
			for (auto it = style.begin(); it != style.end(); ++it)
				format.setProperty(it.key(), it.value());
		};
		for (int i=0; i<m_blocks.size(); ++i) {
			const auto &block = m_blocks[i];
			auto &layout = m_layouts[i];
			QList<QTextLayout::FormatRange> ranges;
			for (auto &format : block.formats) {
				ranges.push_back(QTextLayout::FormatRange());
				auto &range = ranges.last();
				range.start = format.begin;
				range.length = format.end - format.begin;
				mergeFormat(range.format, format.style);
			}
			layout->block.setAdditionalFormats(ranges);

			Q_ASSERT(block.rubies.size() == layout->rubies.size());

			int idx = 0;
			for (const auto &ruby : block.rubies) {
				QList<QTextLayout::FormatRange> ranges;
				ranges << QTextLayout::FormatRange();
				ranges.last().start = 0;
				ranges.last().length = ruby.rt.size();
				mergeFormat(ranges.last().format, ruby.rb_style);
				const int px = ranges.last().format.intProperty(QTextFormat::FontPixelSize)*0.45 + 0.5;
				ranges.last().format.setProperty(QTextFormat::FontPixelSize, px);
				layout->rubies[idx++]->setAdditionalFormats(ranges);
			}
		}
	}
	m_blockChanged = m_formatChanged = m_pxChanged = m_optionChanged = false;
}

void RichTextDocument::draw(QPainter *painter, const QPointF &pos) {
	for (auto layout : m_layouts) {
		layout->block.draw(painter, pos);
		for (auto ruby : layout->rubies)
			ruby->draw(painter, pos);
	}
}
