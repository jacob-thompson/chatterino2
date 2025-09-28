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

QRectF MarkdownLabel::textRect() const
{
    return QRectF(this->rect()).marginsRemoved(this->currentPadding_);
}

void MarkdownLabel::updateSize()
{
    this->currentPadding_ = this->basePadding_.toMarginsF() * this->scale();

    auto yPadding = this->currentPadding_.top() + this->currentPadding_.bottom();

    this->document_->setDefaultFont(
        getApp()->getFonts()->getFont(this->getFontStyle(), this->scale()));
    this->document_->setMarkdown(this->text_);

    // Use word wrap if enabled, otherwise use a reasonable default
    qreal testWidth = this->wordWrap_
                          ? 400.0 * this->scale()
                          : this->document_->idealWidth();
    this->document_->setTextWidth(testWidth);

    auto height = this->document_->size().height() + yPadding;
    auto width = qMin(this->document_->idealWidth(), testWidth) +
                 this->currentPadding_.left() +
                 this->currentPadding_.right();
}

}  // namespace chatterino
