// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "build/pch/pch.h"
#include "fluid2gui.h"
#include "audio/midi/synthesizer.h"
#include "audio/midi/fluid/fluid.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>

namespace Ms {
Fluid2Gui::Fluid2Gui(Synthesizer* synth) : SynthesizerGui(synth)
      {
      auto* layout = new QVBoxLayout(this);
      auto* description = new QLabel(tr("Fonts at the top take priority for matching instruments."), this);
      description->setWordWrap(true);
      layout->addWidget(description);
      auto* row = new QHBoxLayout;
      fonts = new QListWidget(this);
      fonts->setObjectName("fluid2SoundFonts");
      fonts->setAccessibleName(tr("SoundFonts"));
      fonts->setMinimumHeight(150);
      row->addWidget(fonts, 1);
      auto* buttons = new QVBoxLayout;
      const auto button = [&](const QString& text, const char* name) {
            auto* result = new QPushButton(text, this);
            result->setObjectName(name);
            buttons->addWidget(result);
            return result;
            };
      top = button(tr("Top"), "fluid2FontTop");
      up = button(tr("Up"), "fluid2FontUp");
      down = button(tr("Down"), "fluid2FontDown");
      bottom = button(tr("Bottom"), "fluid2FontBottom");
      buttons->addStretch();
      remove = button(tr("Remove"), "fluid2FontRemove");
      remove->setToolTip(tr("Remove the selected SoundFont from this list."));
      add = button(tr("Add…"), "fluid2FontAdd");
      row->addLayout(buttons);
      layout->addLayout(row);
      auto* overrideNotice = new QLabel(tr("SoundFont selection is fixed by the launch environment."), this);
      overrideNotice->setWordWrap(true);
      overrideNotice->setToolTip(tr("Unset MSCORE_FLUID2_SOUNDFONT to edit this list."));
      overrideNotice->setVisible(!qgetenv("MSCORE_FLUID2_SOUNDFONT").isEmpty());
      layout->addWidget(overrideNotice);
      connect(fonts, &QListWidget::currentRowChanged, this, [this]() { updateButtons(); });
      connect(top, &QPushButton::clicked, this, [this]() { move(0); });
      connect(up, &QPushButton::clicked, this, [this]() { move(fonts->currentRow() - 1); });
      connect(down, &QPushButton::clicked, this, [this]() { move(fonts->currentRow() + 1); });
      connect(bottom, &QPushButton::clicked, this, [this]() { move(fonts->count() - 1); });
      connect(remove, &QPushButton::clicked, this, [this]() {
            const int selected = fonts->currentRow();
            if (selected < 0) return;
            QStringList files = paths();
            files.removeAt(selected);
            apply(files, std::min(selected, files.size() - 1));
            });
      connect(add, &QPushButton::clicked, this, [this]() {
            QDialog dialog(this);
            dialog.setObjectName("fluid2SoundFontFilesDialog");
            dialog.setWindowTitle(tr("SoundFont Files"));
            dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
            auto* layout = new QVBoxLayout(&dialog);
            auto* available = new QListWidget(&dialog);
            available->setObjectName("fluid2AvailableSoundFonts");
            available->setAccessibleName(tr("Available SoundFonts"));
            available->setSelectionMode(QAbstractItemView::ExtendedSelection);
            for (const QFileInfo& file : FluidS::Fluid::sfFiles()) {
                  auto* item = new QListWidgetItem(file.fileName(), available);
                  item->setData(Qt::UserRole, file.canonicalFilePath());
                  item->setToolTip(file.absoluteFilePath());
                  }
            layout->addWidget(available);
            auto* box = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
            auto* load = box->addButton(tr("Load"), QDialogButtonBox::AcceptRole);
            load->setEnabled(false);
            layout->addWidget(box);
            connect(available, &QListWidget::itemSelectionChanged, &dialog,
                  [available, load]() { load->setEnabled(!available->selectedItems().isEmpty()); });
            connect(box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            connect(available, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);
            if (available->count()) available->setCurrentRow(0);
            dialog.resize(500, 300);
            if (dialog.exec() != QDialog::Accepted) return;
            QStringList files = paths(), additions;
            for (const QListWidgetItem* item : available->selectedItems()) {
                  const QString canonical = item->data(Qt::UserRole).toString();
                  if (!canonical.isEmpty() && !files.contains(canonical) && !additions.contains(canonical)) additions.append(canonical);
                  }
            if (!additions.isEmpty()) apply(additions + files, 0);
            });
      refresh(0);
      }

QStringList Fluid2Gui::paths() const
      {
      QStringList result;
      for (int row = 0; row < fonts->count(); ++row) result.append(fonts->item(row)->data(Qt::UserRole).toString());
      return result;
      }

void Fluid2Gui::refresh(int row)
      {
      fonts->clear();
      for (const SoundFontInfo& info : synthesizer()->soundFontsInfo()) {
            auto* item = new QListWidgetItem(QFileInfo(info.fileName).fileName(), fonts);
            item->setData(Qt::UserRole, info.fileName);
            item->setToolTip(info.fileName);
            }
      fonts->setCurrentRow(fonts->count() ? std::max(0, std::min(row, fonts->count() - 1)) : -1);
      updateButtons();
      }

void Fluid2Gui::updateButtons()
      {
      const bool enabled = !busy && qgetenv("MSCORE_FLUID2_SOUNDFONT").isEmpty();
      const int row = fonts->currentRow(), count = fonts->count();
      fonts->setEnabled(!busy);
      add->setEnabled(enabled);
      remove->setEnabled(enabled && row >= 0);
      top->setEnabled(enabled && row > 0);
      up->setEnabled(enabled && row > 0);
      down->setEnabled(enabled && row >= 0 && row + 1 < count);
      bottom->setEnabled(enabled && row >= 0 && row + 1 < count);
      }

void Fluid2Gui::move(int target)
      {
      const int selected = fonts->currentRow();
      if (selected < 0 || target < 0 || target >= fonts->count() || selected == target) return;
      QStringList files = paths();
      files.move(selected, target);
      apply(files, target);
      }

void Fluid2Gui::apply(const QStringList& files, int row)
      {
      if (busy || !qgetenv("MSCORE_FLUID2_SOUNDFONT").isEmpty() || files == paths()) return;
      const int previous = fonts->currentRow();
      busy = true;
      updateButtons();
      // The host stops transport before worker-side loading can take the
      // synth lock. There is no automatic restart of the old performance.
      emit soundFontChangeRequested();
      QProgressDialog progress(tr("Loading SoundFonts…"), QString(), 0, 0, this);
      progress.setCancelButton(nullptr);
      progress.setWindowModality(Qt::ApplicationModal);
      progress.setAutoClose(false);
      progress.setAutoReset(false);
      QFutureWatcher<bool> watcher;
      connect(&watcher, &QFutureWatcher<bool>::finished, &progress, &QDialog::accept, Qt::QueuedConnection);
      Synthesizer* synth = synthesizer();
      const QFuture<bool> future = QtConcurrent::run([synth, files]() { return synth->loadSoundFonts(files); });
      watcher.setFuture(future);
      progress.exec();
      // Closing the progress window cannot cancel a FluidSynth load. Always
      // finish before returning, so no job can outlive this operation/backend.
      watcher.waitForFinished();
      const bool loaded = watcher.result();
      busy = false;
      refresh(loaded ? row : previous);
      if (loaded) {
            emit sfChanged();
            emit valueChanged();
            }
      else QMessageBox::warning(this, tr("SoundFonts"),
            tr("Could not load the requested SoundFonts. The previous list is unchanged."));
      }

void Fluid2Gui::synthesizerChanged()
      {
      if (busy) return;
      refresh(fonts->currentRow());
      emit sfChanged();
      }
}
