//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2007 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "globals.h"
#include "icons.h"
#include "libmscore/score.h"
#include "libmscore/style.h"
#include "preferences.h"
#include "libmscore/sym.h"
#include "libmscore/mscore.h"
#include "miconengine.h"

namespace Ms {

extern QString iconPath;
QIcon* icons[int(Icons::ICONS)];

//---------------------------------------------------------
//   genIcons
//    create some icons
//    keep in syn with enum class Icons in icons.h
//---------------------------------------------------------

static const char* iconNames[] = {
      "empty.svg",
      "options.svg",
      "note-longa.svg",
      "note-breve.svg",
      "note-1.svg",
      "note-2.svg",
      "note-4.svg",
      "note-8.svg",
      "note-16.svg",
      "note-32.svg",
      "note-64.svg",
      "note-128.svg",
      "note-256.svg",
      "note-512.svg",
      "note-1024.svg",
      "note-natural.svg",
      "note-sharp.svg",
      "note-double-sharp.svg",
      "note-flat.svg",
      "note-double-flat.svg",
      "rest.svg",
      "note-dot.svg",
      "note-double-dot.svg",
      "note-dot3.svg",
      "note-dot4.svg",
      "stem-flip.svg",
      "mouse-entry.svg",
      "edit-playback.svg",
      "edit-undo.svg",
      "edit-redo.svg",
      "edit-cut.svg",
      "edit-copy.svg",
      "edit-paste.svg",
      "edit-swap.svg",
      "document-print.svg",
      "clef.svg",
      "midi-input.svg",
      "sound-while-editing.svg",
      "media-skip-backward.svg",
      "media-playback-start.svg",
      "media-playback-repeats.svg",
      "media-playback-pan.svg",
      "sbeam.svg",
      "mbeam.svg",
      "nbeam.svg",
      "beam32.svg",
      "beam64.svg",
      "default.svg",
      "fbeam1.svg",
      "fbeam2.svg",
      "document.svg",
      "document-open.svg",
      "document-new.svg",
      "document-save.svg",
      "document-save-as.svg",
      "document-save-online.svg",
      "mscore.png",
      "acciaccatura.svg",
      "appoggiatura.svg",
      "grace4.svg",
      "grace16.svg",
      "grace32.svg",
      "grace8after.svg",
      "grace16after.svg",
      "grace32after.svg",
      "mode-notes.svg",
      // "mode-notes-steptime.svg", (using normal icon for the time being.)
      "mode-notes-repitch.svg",
      "mode-notes-rhythm.svg",
      "mode-notes-realtime-auto.svg",
      "mode-notes-realtime-manual.svg",
      "insert-symbol.svg",
      "note-tie.svg",
      "format-text-bold.svg",
      "format-text-italic.svg",
      "format-text-underline.svg",
      "format-text-strike.svg",
      "format-justify-left.svg",
      "format-justify-center.svg",
      "format-justify-right.svg",
      "align-vertical-top.svg",
      "align-vertical-bottom.svg",
      "align-vertical-center.svg",
      "align-vertical-baseline.svg",
      "format-text-superscript.svg",
      "format-text-subscript.svg",
      "mode-photo.svg",
      "raster-horizontal.svg",
      "raster-vertical.svg",
      "list-unordered.svg",
      "list-ordered.svg",
      "format-indent-more.svg",
      "format-indent-less.svg",
      "media-playback-loop.svg",
      "media-playback-loop-in.svg",
      "media-playback-loop-out.svg",
      "media-playback-metronome.svg",
      "media-playback-countin.svg",
      "frame-vertical.svg",
      "frame-horizontal.svg",
      "frame-text.svg",
      "frame-fretboard.svg",
      "measure.svg",
      "object-select.svg",
      "mscz-icon.svg",
      "help-contents.svg",
      "go-home.svg",
      "go-previous.svg",
      "go-next.svg",
      "view-refresh.svg",
      "parentheses.svg",
      "brackets.svg",
      "braces.svg",
      "timesig_allabreve.svg",
      "timesig_common.svg",
      "timesig_prolatio01.svg",
      "timesig_prolatio02.svg",
      "timesig_prolatio03.svg",
      "timesig_prolatio04.svg",
      "timesig_prolatio05.svg",
      "timesig_prolatio07.svg",
      "timesig_prolatio08.svg",
      "timesig_prolatio10.svg",
      "timesig_prolatio11.svg",
      "edit.svg",
      "edit-reset.svg",
      "window-close.svg",
      "arrow_up.svg",
      "arrow_down.svg",
      "mail.svg",
      "bug.svg",
      "bin.svg",
      "note_timewise.svg",
      "arrowsMoveToTop.svg",
      "arrowsMoveToBottom.svg",
      "","","","", // voices
      "color_all.svg",              // override color icons
      "color_noteheads.svg",
      "color_stafflines.svg",
      "color_ledgerlines.svg",
      "color_dynamics.svg",
      "color_fingeringtext.svg",
      "color_stafftext.svg",
      "color_expressiontext.svg",
      "color_harmonytext.svg",
      "color_textlines.svg",
      "color_boxtext.svg",
      "color_slurs.svg",
      "color_ties.svg",
      "color_noteheads_lowered.svg",
      "color_noteheads_raised.svg",
      "color_hover.svg",
      "color_hover.svg",            // Lasso
      "color_grips.svg",
      "color_framemargins.svg",
      "empty.svg",                  // Layout breaks
      "color_invisible.svg",
      "color_pianohighlight.svg",   // Piano normal selection highlighting
      "color_pianohighlight.svg",   // Piano white keys
      "color_pianohighlight.svg",   // Piano black keys
      "color_noteheads.svg",        // Single selection color
      "color_hover.svg",            // Cursor color
      "empty.svg",                  // Barlines color
      "empty.svg",                  // Brackets color (being lazy not creating new icons)
      "empty.svg",                  // Voice-1
      "empty.svg",                  // Voice-2
      "empty.svg",                  // Voice-3
      "empty.svg",                  // Voice-4
      "empty.svg",                  // Note Entry information
      };

//---------------------------------------------------------
//   genIcons
//---------------------------------------------------------

void genIcons()
      {
      for (int i = 0; i < int(Icons::voice1_ICON); ++i) {
            QIcon* icon = new QIcon(new MIconEngine);
            icon->addFile(iconPath + iconNames[i]);
            icons[i] = icon;
            if (icon->isNull() || icon->pixmap(12).isNull()) {
                  qDebug("cannot load Icon <%s>", qPrintable(iconPath + iconNames[i]));
                  }
            }

      static const char* vtext[VOICES] = { "1","2","3","4" };
      int iw = preferences.getInt(PREF_UI_THEME_ICONHEIGHT) * 2 / 3; // 16;
      int ih = preferences.getInt(PREF_UI_THEME_ICONHEIGHT);   // 24;
      for (int i = 0; i < VOICES; ++i) {
            icons[int(Icons::voice1_ICON) + i] = new QIcon;
            QPixmap image(iw, ih);
            QColor c(MScore::selectColor[i].lighter(180));
            image.fill(c);
            QPainter painter(&image);
            painter.setFont(QFont("FreeSans", 8));
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setPen(QPen(Qt::black));
            painter.drawText(QRect(0, 0, iw, ih), Qt::AlignCenter, vtext[i]);
            painter.end();
            icons[int(Icons::voice1_ICON) +i]->addPixmap(image);

            painter.begin(&image);
            c = QColor(MScore::selectColor[i].lighter(140));
            painter.fillRect(0, 0, iw, ih, c);
            painter.setPen(QPen(Qt::black));
            painter.drawText(QRect(0, 0, iw, ih), Qt::AlignCenter, vtext[i]);
            painter.end();
            icons[int(Icons::voice1_ICON) + i]->addPixmap(image, QIcon::Normal, QIcon::On);
            }

      // Color Override Options:
      iw = preferences.getInt(PREF_UI_THEME_ICONWIDTH);
      ih = preferences.getInt(PREF_UI_THEME_ICONHEIGHT);
      QColor c;
      for (int i = static_cast<int>(Icons::overrideColorAll_ICON); i < static_cast<int>(Icons::ICONS); ++i) {
            switch (static_cast<Icons>(i)) {
            case Icons::overrideColorAll_ICON:              c = MScore::overrideAllColor;             break;
            case Icons::overrideColorNoteheads_ICON:        c = MScore::overrideNoteheadColor;        break;
            case Icons::overrideColorStafflines_ICON:       c = MScore::overrideStaffLinesColor;      break;
            case Icons::overrideColorLedgerlines_ICON:      c = MScore::overrideLedgerLinesColor;     break;
            case Icons::overrideColorDynamics_ICON:         c = MScore::overrideDynamicsColor;        break;
            case Icons::overrideColorFingeringtext_ICON:    c = MScore::overrideFingeringTextColor;   break;
            case Icons::overrideColorStafftext_ICON:        c = MScore::overrideStaffTextColor;       break;
            case Icons::overrideColorExpressiontext_ICON:   c = MScore::overrideExpressionTextColor;  break;
            case Icons::overrideColorHarmonytext_ICON:      c = MScore::overrideHarmonyTextColor;     break;
            case Icons::overrideColorTextlines_ICON:        c = MScore::overrideTextLinesColor;       break;
            case Icons::overrideColorBoxtext_ICON:          c = MScore::overrideBoxTextsColor;        break;
            case Icons::overrideColorSlurs_ICON:            c = MScore::overrideSlursColor;           break;
            case Icons::overrideColorTies_ICON:             c = MScore::overrideTiesColor;            break;
            case Icons::overrideColorLowerednoteheads_ICON: c = MScore::overrideNoteheadLoweredColor; break;
            case Icons::overrideColorRaisednoteheads_ICON:  c = MScore::overrideNoteheadRaisedColor;  break;
            case Icons::overrideColorHover_ICON:            c = MScore::hoverColor;                   break;
            case Icons::overrideColorLasso_ICON:            c = MScore::lassoColor;                   break;
            case Icons::overrideColorGrips_ICON:            c = MScore::gripsColor;                   break;
            case Icons::overrideColorFramemargins_ICON:     c = MScore::frameMarginColor;             break;
            case Icons::overrideColorInvisible_ICON:        c = MScore::invisibleElementsColor;       break;
            case Icons::overrideColorLayoutBreaks_ICON:     c = MScore::layoutBreakColor;             break;
            case Icons::overrideColorPianohighlight_ICON:   c = MScore::pianoHighlightColor;          break;
            case Icons::overrideColorPianoWhiteKeys_ICON:   c = MScore::pianoWhiteKeysColor;          break;
            case Icons::overrideColorPianoBlackKeys_ICON:   c = MScore::pianoBlackKeysColor;          break;
            case Icons::overrideColorSingleSelection_ICON:  c = MScore::singleNoteSelectionColor;     break;
            case Icons::overrideColorCursor_ICON:           c = MScore::cursorColor;                  break;
            case Icons::overrideColorBarlines_ICON:         c = MScore::overrideBarlinesColor;        break;
            case Icons::overrideColorBrackets_ICON:         c = MScore::overrideBracketsColor;        break;
            case Icons::overrideColorVoice1_ICON:           c = MScore::selectColor[0];               break;
            case Icons::overrideColorVoice2_ICON:           c = MScore::selectColor[1];               break;
            case Icons::overrideColorVoice3_ICON:           c = MScore::selectColor[2];               break;
            case Icons::overrideColorVoice4_ICON:           c = MScore::selectColor[3];               break;
            case Icons::overrideColorNoteEntryStatus_ICON:  c = MScore::noteEntryInformationColor;    break;

            default:
                  c = MScore::defaultColor;
                  break;
                  }

            icons[i] = new QIcon();
            QPixmap colorMap(iw, ih);
            QPixmap iconMap(iconPath + iconNames[i]);
            colorMap.fill(c);

            QPainter painter(&colorMap);
            QImage imageMap = iconMap.toImage();
            if (c.lightness() <= 100)
                  imageMap.invertPixels();
            painter.drawImage(2,0, imageMap);
            painter.end();

            icons[i]->addPixmap(colorMap);
            }
      }
}

