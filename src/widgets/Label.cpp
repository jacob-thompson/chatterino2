#include "widgets/Label.hpp"

#include "Application.hpp"

#include <QPainter>
#include <QTextDocument>
#include <QRegExp>

namespace chatterino {

Label::Label(QString text, FontStyle style)
    : Label(nullptr, std::move(text), style)
{
}

Label::Label(BaseWidget *parent, QString text, FontStyle style)
    : BaseWidget(parent)
    , text_(std::move(text))
    , fontStyle_(style)
    , basePadding_(8, 0, 8, 0)
{
    this->connections_.managedConnect(getApp()->getFonts()->fontChanged,
                                      [this] {
                                          this->updateSize();
                                      });
    this->updateSize();
}

const QString &Label::getText() const
{
    return this->text_;
}

void Label::setText(const QString &text)
{
    if (this->text_ != text)
    {
        this->text_ = text;
        if (this->shouldElide_)
        {
            this->updateElidedText(this->getFontMetrics(),
                                   this->textRect().width());
        }
        if (this->markdownEnabled_ && this->markdownDocument_)
        {
            this->markdownDocument_->setMarkdown(text);
        }
        this->updateSize();
        this->update();
    }
}

FontStyle Label::getFontStyle() const
{
    return this->fontStyle_;
}

bool Label::getCentered() const
{
    return this->centered_;
}

void Label::setCentered(bool centered)
{
    this->centered_ = centered;
    this->updateSize();
}

void Label::setPadding(QMargins padding)
{
    this->basePadding_ = padding;
    this->updateSize();
}

bool Label::getWordWrap() const
{
    return this->wordWrap_;
}

void Label::setWordWrap(bool wrap)
{
    this->wordWrap_ = wrap;
    this->update();
}

void Label::setShouldElide(bool shouldElide)
{
    this->shouldElide_ = shouldElide;
    this->updateSize();
    this->update();
}

bool Label::getMarkdownEnabled() const
{
    return this->markdownEnabled_;
}

void Label::setMarkdownEnabled(bool enabled)
{
    this->markdownEnabled_ = enabled;
    this->updateSize();
    this->update();
}

void Label::setTextOrHtml(const QString &text)
{
    if (this->markdownEnabled_)
    {
        // Convert markdown to HTML and set it
        QString html = this->markdownToHtml(text);
        this->setText(html);
    }
    else
    {
        // Set plain text normally
        this->setText(text);
    }
}

QString Label::markdownToHtml(const QString &markdown) const
{
    if (markdown.isEmpty())
    {
        return QString();
    }
    
    // Create a temporary QTextDocument to convert markdown to HTML
    QTextDocument tempDoc;
    tempDoc.setMarkdown(markdown);
    
    // Get the HTML representation
    QString html = tempDoc.toHtml();
    
    // Clean up the HTML to remove unnecessary tags and styling
    // Remove document structure tags and keep only the content
    QRegExp bodyRegex("<body[^>]*>(.*)</body>", Qt::CaseInsensitive);
    bodyRegex.setMinimal(true);
    if (bodyRegex.indexIn(html) != -1)
    {
        html = bodyRegex.cap(1);
    }
    
    return html;
}

void Label::setFontStyle(FontStyle style)
{
    this->fontStyle_ = style;
    this->updateSize();
}

void Label::scaleChangedEvent(float /*scale*/)
{
    this->updateSize();
}

QSize Label::sizeHint() const
{
    return this->sizeHint_;
}

QSize Label::minimumSizeHint() const
{
    return this->minimumSizeHint_;
}

void Label::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    auto metrics = this->getFontMetrics();

    painter.setFont(
        getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

    // draw text
    QRectF textRect = this->textRect();

    if (this->markdownEnabled_ && !this->text_.isEmpty())
    {
        // Use QTextDocument to render HTML (converted from markdown)
        QTextDocument doc;
        doc.setDefaultFont(getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));
        doc.setTextWidth(textRect.width());
        
        // Set HTML content (assuming text_ contains HTML converted from markdown)
        doc.setHtml(this->text_);
        
        // Set text color to match palette
        QString colorName = this->palette().windowText().color().name();
        doc.setDefaultStyleSheet(
            QString("body { color: %1; margin: 0; padding: 0; } p { margin: 0; } h1, h2, h3, h4, h5, h6 { margin: 0; }")
                .arg(colorName));
        
        // Save painter state and translate to text rect position
        painter.save();
        painter.translate(textRect.topLeft());
        
        // Draw the document
        doc.drawContents(&painter, QRectF(0, 0, textRect.width(), textRect.height()));
        
        painter.restore();
    }
    else
    {
        // Original text rendering code for plain text
        auto text = [this] {
            if (this->shouldElide_)
            {
                return this->elidedText_;
            }

            return this->text_;
        }();

        qreal width = metrics.horizontalAdvance(text);
        Qt::Alignment alignment = !this->centered_ || width > textRect.width()
                                      ? Qt::AlignLeft | Qt::AlignVCenter
                                      : Qt::AlignCenter;

        painter.setBrush(this->palette().windowText());

        QTextOption option(alignment);
        if (this->wordWrap_)
        {
            option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        }
        else
        {
            option.setWrapMode(QTextOption::NoWrap);
        }
        
        painter.drawText(textRect, text, option);
    }

#if 0
    painter.setPen(QColor(255, 0, 0));
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
#endif
}

void Label::resizeEvent(QResizeEvent *event)
{
    if (this->shouldElide_)
    {
        auto metrics = this->getFontMetrics();
        if (this->updateElidedText(metrics, this->textRect().width()))
        {
            this->update();
        }
    }

    BaseWidget::resizeEvent(event);
}

QFontMetricsF Label::getFontMetrics() const
{
    return getApp()->getFonts()->getFontMetrics(this->fontStyle_,
                                                this->scale());
}

void Label::updateSize()
{
    this->currentPadding_ = this->basePadding_.toMarginsF() * this->scale();

    auto metrics = this->getFontMetrics();

    auto yPadding =
        this->currentPadding_.top() + this->currentPadding_.bottom();
    
    if (this->markdownEnabled_ && !this->text_.isEmpty())
    {
        // Size based on HTML content using QTextDocument
        QTextDocument doc;
        doc.setDefaultFont(getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));
        doc.setHtml(this->text_);
        
        // Use word wrap width if enabled, otherwise use a reasonable default
        qreal testWidth = this->wordWrap_ ? 400.0 * this->scale() : doc.idealWidth();
        doc.setTextWidth(testWidth);
        
        auto height = doc.size().height() + yPadding;
        auto width = qMin(doc.idealWidth(), testWidth) +
                     this->currentPadding_.left() +
                     this->currentPadding_.right();
        
        this->sizeHint_ = QSizeF(width, height).toSize();
        this->minimumSizeHint_ = this->sizeHint_;
    }
    else
    {
        // Original sizing logic for plain text
        auto height = metrics.height() + yPadding;
        if (this->shouldElide_)
        {
            this->updateElidedText(metrics, this->textRect().width());
            this->sizeHint_ = QSizeF(-1, height).toSize();
            this->minimumSizeHint_ = this->sizeHint_;
        }
        else
        {
            auto width = metrics.horizontalAdvance(this->text_) +
                         this->currentPadding_.left() +
                         this->currentPadding_.right();
            this->sizeHint_ = QSizeF(width, height).toSize();
            this->minimumSizeHint_ = this->sizeHint_;
        }
    }

    this->updateGeometry();
}

bool Label::updateElidedText(const QFontMetricsF &fontMetrics, qreal width)
{
    assert(this->shouldElide_ == true);
    auto elidedText = fontMetrics.elidedText(
        this->text_, Qt::TextElideMode::ElideRight, width);

    if (elidedText != this->elidedText_)
    {
        this->elidedText_ = elidedText;
        return true;
    }

    return false;
}

QRectF Label::textRect() const
{
    return this->rect().toRectF().marginsRemoved(this->currentPadding_);
}

}  // namespace chatterino
