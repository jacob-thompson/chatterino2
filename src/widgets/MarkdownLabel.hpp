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
    void scaleChangedEvent(float scale) override;
    void resizeEvent(QResizeEvent *event) override;
    void updateSize() override;

private:
    void ensureDocumentUpdated() const;
    void updateDocumentSize() const;
    mutable std::unique_ptr<QTextDocument> document_;
    mutable QString lastText_;
};

}  // namespace chatterino
