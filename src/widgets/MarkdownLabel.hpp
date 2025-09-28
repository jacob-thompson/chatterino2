#pragma once

#include "widgets/Label.hpp"

#include <memory>

class QTextDocument;

namespace chatterino {

class MarkdownLabel : public Label
{
public:
    using Label::Label;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRectF textRect() const;
    void updateSize();
    mutable std::unique_ptr<QTextDocument> document_;
    QMargins basePadding_;
    QMarginsF currentPadding_;
    QString text_;
    bool wordWrap_ = true;
};

}  // namespace chatterino
