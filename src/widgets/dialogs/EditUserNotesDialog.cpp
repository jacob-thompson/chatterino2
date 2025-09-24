#include "EditUserNotesDialog.hpp"

#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Label.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QSplitter>
#include <QTextEdit>

namespace chatterino {

EditUserNotesDialog::EditUserNotesDialog(QWidget *parent)
    : BasePopup(
          {
              BaseWindow::EnableCustomFrame,
              BaseWindow::BoundsCheckOnShow,
          },
          parent)
{
    this->setScaleIndependentSize(700, 450);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    auto previewCheckBox = layout.emplace<QCheckBox>("Show Markdown Preview")
                               .assign(&this->previewCheckBox_);

    auto splitter =
        layout.emplace<QSplitter>(Qt::Horizontal).assign(&this->splitter_);

    auto edit = splitter.emplace<QTextEdit>().assign(&this->textEdit_);

    auto preview = splitter.emplace<Label>().assign(&this->previewLabel_);
    preview->setMarkdownEnabled(true);
    preview->setWordWrap(true);
    preview->setPadding(QMargins(10, 10, 10, 10));

    // Restore splitter sizes and preview visibility from settings
    auto &settings = getSettings()->editUserNotesDialog;
    bool showPreview = settings.showMarkdownPreview;
    auto sizes = settings.splitterSizes.getValue();
    
    // Ensure we have valid sizes
    if (sizes.size() != 2 || (sizes[0] <= 0 && sizes[1] <= 0))
    {
        sizes = {350, 350};
    }
    
    this->previewCheckBox_->setChecked(showPreview);
    this->previewLabel_->setVisible(showPreview);
    
    if (showPreview)
    {
        this->splitter_->setSizes(sizes);
    }
    else
    {
        this->splitter_->setSizes({700, 0});
    }

    layout
        .emplace<QDialogButtonBox>(QDialogButtonBox::Ok |
                                   QDialogButtonBox::Cancel)
        .connect(&QDialogButtonBox::accepted, this,
                 [this, edit = edit.getElement()] {
                     this->onOk.invoke(edit->toPlainText());
                     this->close();
                 })
        .connect(&QDialogButtonBox::rejected, this, [this] {
            this->close();
        });

    // Connect preview toggle
    QObject::connect(this->previewCheckBox_, &QCheckBox::toggled, this,
                     [this](bool checked) {
                         auto &settings = getSettings()->editUserNotesDialog;
                         settings.showMarkdownPreview = checked;
                         
                         this->previewLabel_->setVisible(checked);
                         if (checked)
                         {
                             this->updatePreview();
                             // Restore saved splitter sizes or use defaults
                             auto sizes = settings.splitterSizes.getValue();
                             if (sizes.size() == 2 && sizes[0] > 0 && sizes[1] > 0)
                             {
                                 this->splitter_->setSizes(sizes);
                             }
                             else
                             {
                                 this->splitter_->setSizes({350, 350});
                             }
                         }
                         else
                         {
                             // Save current splitter sizes before hiding
                             if (this->previewLabel_->isVisible())
                             {
                                 auto currentSizes = this->splitter_->sizes();
                                 if (currentSizes.size() == 2)
                                 {
                                     std::vector<int> sizesVec = {currentSizes[0], currentSizes[1]};
                                     settings.splitterSizes = sizesVec;
                                 }
                             }
                             this->splitter_->setSizes({700, 0});
                         }
                     });

    // Connect text changes to preview update
    QObject::connect(this->textEdit_, &QTextEdit::textChanged, this, [this] {
        if (this->previewCheckBox_->isChecked())
        {
            this->updatePreview();
        }
    });

    // Save splitter sizes when user drags the splitter
    QObject::connect(this->splitter_, &QSplitter::splitterMoved, this, [this] {
        this->saveSplitterSizes();
    });

    this->themeChangedEvent();
}

void EditUserNotesDialog::closeEvent(QCloseEvent *event)
{
    this->saveSplitterSizes();
    BasePopup::closeEvent(event);
}

void EditUserNotesDialog::setNotes(const QString &notes)
{
    this->textEdit_->setPlainText(notes);
}

void EditUserNotesDialog::updateWindowTitle(const QString &displayUsername)
{
    this->setWindowTitle("Editing notes for " + displayUsername);
}

void EditUserNotesDialog::showEvent(QShowEvent *event)
{
    this->textEdit_->setFocus(Qt::FocusReason::ActiveWindowFocusReason);

    BasePopup::showEvent(event);
}

void EditUserNotesDialog::themeChangedEvent()
{
    if (!this->theme)
    {
        return;
    }

    auto palette = this->palette();

    palette.setColor(QPalette::Window,
                     this->theme->tabs.selected.backgrounds.regular);
    palette.setColor(QPalette::Base, getTheme()->splits.background);
    palette.setColor(QPalette::Text, getTheme()->window.text);

    this->setPalette(palette);

    if (this->textEdit_)
    {
        this->textEdit_->setPalette(palette);
    }

    if (this->previewLabel_)
    {
        this->previewLabel_->setPalette(palette);
    }
}

void EditUserNotesDialog::updatePreview()
{
    if (this->previewLabel_ && this->textEdit_)
    {
        QString text = this->textEdit_->toPlainText();
        if (text.isEmpty())
        {
            this->previewLabel_->setText(
                "*Preview will appear here when you type markdown text...*");
        }
        else
        {
            this->previewLabel_->setText(text);
        }
    }
}

void EditUserNotesDialog::saveSplitterSizes()
{
    if (this->previewCheckBox_->isChecked() && this->previewLabel_->isVisible())
    {
        auto sizes = this->splitter_->sizes();
        if (sizes.size() == 2 && sizes[0] > 0 && sizes[1] > 0)
        {
            std::vector<int> sizesVec = {sizes[0], sizes[1]};
            getSettings()->editUserNotesDialog.splitterSizes = sizesVec;
        }
    }
}

}  // namespace chatterino
