//=============================================================================
//  MuseScore
//  Copyright (C) 2026 MuseScore contributors
//  SPDX-License-Identifier: GPL-2.0-only
//=============================================================================
#ifndef MS_GUITARTECHNIQUE_H
#define MS_GUITARTECHNIQUE_H
#include "slur.h"
#include "textline.h"
namespace Ms {

// Retain distinct Studio file types while sharing 3.x's established editors.
class HammerOnPullOff final : public Slur {
   public:
      explicit HammerOnPullOff(Score* s) : Slur(s) {}
      HammerOnPullOff* clone() const override { return new HammerOnPullOff(*this); }
      ElementType type() const override { return ElementType::HAMMER_ON_PULL_OFF; }
      };

class WhammyBar final : public TextLine {
   public:
      explicit WhammyBar(Score* s) : TextLine(s) {
            setBeginText("w/bar");
            setContinueText("(w/bar)");
            setLineStyle(Qt::DashLine);
            setEndHookType(HookType::HOOK_90T);
            setBeginHookHeight(Spatium(0.6));
            setEndHookHeight(Spatium(0.6));
            }
      WhammyBar* clone() const override { return new WhammyBar(*this); }
      ElementType type() const override { return ElementType::WHAMMY_BAR; }
      QVariant propertyDefault(Pid id) const override {
            switch (id) {
                  case Pid::END_HOOK_TYPE:
                        return int(HookType::HOOK_90T);
                  case Pid::BEGIN_HOOK_HEIGHT:
                  case Pid::END_HOOK_HEIGHT:
                        return Spatium(0.6);
                  case Pid::BEGIN_TEXT:
                        return QString("w/bar");
                  case Pid::CONTINUE_TEXT:
                        return QString("(w/bar)");
                  case Pid::LINE_STYLE:
                        return int(Qt::DashLine);
                  default:
                        return TextLine::propertyDefault(id);
                  }
            }
      };
} // namespace Ms
#endif
