#include "widgets/MarkdownLabel.hpp"

#include "Application.hpp"

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QPainter>
#include <QTextDocument>
#include <QUrl>

namespace chatterino {

void MarkdownLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        this->ensureDocumentUpdated();

        if (!this->document_)
        {
            Label::mousePressEvent(event);
            return;
        }

        QRectF textRect = this->textRect();
        QPointF pos = event->pos() - textRect.topLeft();

        QString anchor = this->document_->documentLayout()->anchorAt(pos);
        if (!anchor.isEmpty())
        {
            QUrl url(anchor);

            if (!url.isValid())
            {
                return;
            }

            if (url.scheme().isEmpty())
            {
                url.setScheme("http");
            }

            QString scheme = url.scheme().toLower();
            if (scheme == "http" || scheme == "https" || scheme == "mailto" ||
                scheme == "file" || scheme == "ftp")
            {
                QDesktopServices::openUrl(url);
            }
            return;
        }
    }

    BaseWidget::mousePressEvent(event);
}

void MarkdownLabel::mouseMoveEvent(QMouseEvent *event)
{
    this->ensureDocumentUpdated();

    if (!this->document_)
    {
        Label::mouseMoveEvent(event);
        return;
    }

    QRectF textRect = this->textRect();
    QPointF pos = event->pos() - textRect.topLeft();

    QString anchor = this->document_->documentLayout()->anchorAt(pos);
    if (!anchor.isEmpty())
    {
        this->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        this->setCursor(Qt::ArrowCursor);
    }

    BaseWidget::mouseMoveEvent(event);
}

void MarkdownLabel::paintEvent(QPaintEvent *event)
{
    this->ensureDocumentUpdated();

    if (!this->document_)
    {
        // Fallback to base class rendering if document failed
        Label::paintEvent(event);
        return;
    }

    QPainter painter(this);

    painter.setFont(getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));

    QRectF textRect = this->textRect();

    painter.setBrush(this->palette().windowText());
    this->document_->setTextWidth(textRect.width());

    painter.save();
    painter.translate(textRect.topLeft());

    this->document_->drawContents(&painter, QRectF(0, 0, textRect.width(), textRect.height()));

    painter.restore();
}

void MarkdownLabel::scaleChangedEvent(float scale)
{
    // Reset document so it gets recreated with new scale
    this->document_.reset();
    this->lastText_.clear();
    Label::scaleChangedEvent(scale);
}

void MarkdownLabel::resizeEvent(QResizeEvent *event)
{
    // Update document width when resized
    this->updateDocumentSize();

    // Call base class but skip eliding logic for markdown
    BaseWidget::resizeEvent(event);
}

void MarkdownLabel::updateSize()
{
    // Update padding from base class
    this->currentPadding_ = this->basePadding_.toMarginsF() * this->scale();

    // Update document first
    this->ensureDocumentUpdated();

    if (this->document_)
    {
        // Set a reasonable width for the document based on word wrap
        qreal testWidth = this->wordWrap_
                              ? 400.0 * this->scale()
                              : this->document_->idealWidth();
        this->document_->setTextWidth(testWidth);

        // Calculate size based on document
        auto yPadding = this->currentPadding_.top() + this->currentPadding_.bottom();
        auto height = this->document_->size().height() + yPadding;
        auto width = qMin(this->document_->idealWidth(), testWidth) +
                     this->currentPadding_.left() +
                     this->currentPadding_.right();

        this->sizeHint_ = QSizeF(width, height).toSize();
        this->minimumSizeHint_ = this->sizeHint_;
    }
    else
    {
        // Fallback to base class calculation if no document
        Label::updateSize();
        return;
    }

    this->updateGeometry();
}

void MarkdownLabel::ensureDocumentUpdated() const
{
    const QString &currentText = this->text_;

    // Don't create document for empty text
    if (currentText.isEmpty())
    {
        this->document_.reset();
        this->lastText_.clear();
        return;
    }

    if (!this->document_ || this->lastText_ != currentText)
    {
        if (!this->document_)
        {
            this->document_ = std::make_unique<QTextDocument>();
        }

        this->document_->setDefaultFont(
            getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));
        this->document_->setMarkdown(currentText);

        this->lastText_ = currentText;
    }
}

void MarkdownLabel::updateDocumentSize() const
{
    /*
    if (this->document_)
    {
        // Set text width based on current widget size, but with a minimum
        QRectF textRect = this->textRect();
        qreal width = qMax(textRect.width(), 100.0 * this->scale());
        this->document_->setTextWidth(width);
    }
    */
}

}  // namespace chatterino
