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

    Label::mousePressEvent(event);
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

    Label::mouseMoveEvent(event);
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
    if (this->document_)
    {
        QRectF textRect = this->textRect();
        this->document_->setTextWidth(textRect.width());
    }
    
    // Call base class but skip eliding logic for markdown
    BaseWidget::resizeEvent(event);
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
        
        // Set text width based on current widget size
        QRectF textRect = this->textRect();
        this->document_->setTextWidth(textRect.width());
        
        this->lastText_ = currentText;
    }
}

}  // namespace chatterino
