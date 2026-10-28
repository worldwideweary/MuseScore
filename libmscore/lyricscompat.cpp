// Copyright (C) 2026 MuseScore contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <QDomDocument>

#include "lyricscompat.h"
#include "chordrest.h"
#include "lyrics.h"
#include "measure.h"
#include "score.h"
#include "segment.h"
#include "utils.h"

namespace Ms {

static bool hasFollowingRepeatBoundary(const Measure* measure)
      {
      if (measure->repeatEnd() || !followingRepeatMeasures(measure).empty())
            return true;
      // Before the first layout, volta end anchors are not yet resolved for
      // the roadmap. A volta beginning at the next bar is already explicit.
      for (const auto& entry : measure->score()->spanner()) {
            if (entry.second->isVolta() && entry.second->tick() == measure->endTick())
                  return true;
            }
      return false;
      }

// Read native 4.x line metadata after ordinary score reading has established
// all chord/rest times (including tuplets). The original XML is never rewritten.
static ChordRest* endChordRest(ChordRest* first, const Fraction& end, bool exclusive)
      {
      ChordRest* result = first;
      for (Segment* s = first->segment(); s; s = s->next1(SegmentType::ChordRest)) {
            if (exclusive ? s->tick() >= end : s->tick() > end)
                  break;
            Element* item = s->element(first->track());
            if (item && item->isChordRest())
                  result = toChordRest(item);
            }
      return result;
      }

static bool isGrace(const QDomElement& chord)
      {
      for (QDomElement child = chord.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
            const QString name = child.tagName();
            if (name == "acciaccatura" || name == "appoggiatura" || name.startsWith("grace"))
                  return true;
            }
      return false;
      }

static void importIncomingLine(MasterScore* score, const std::vector<Measure*>& measures, int index,
                               int track, const Fraction& relativeTick, const QDomElement& record)
      {
      const QDomElement properties = record.firstChildElement("PartialLyricsLine");
      const QDomElement location = record.firstChildElement("next").firstChildElement("location");
      if (properties.isNull() || location.isNull())
            return; // endpoint record, rather than the line's definition
      if (location.firstChildElement("staves").text().toInt() || location.firstChildElement("voices").text().toInt())
            return; // a cross-track line cannot be represented by one 3.x lyric owner
      const int endIndex = index + location.firstChildElement("measures").text().toInt();
      if (endIndex < 0 || endIndex > int(measures.size()))
            return;
      const QString fraction = location.firstChildElement("fractions").text();
      const Fraction relativeEnd = relativeTick + (fraction.isEmpty() ? Fraction(0, 1) : Fraction::fromString(fraction));
      const Fraction end = endIndex == int(measures.size()) ? measures.back()->endTick() + relativeEnd
                                                          : measures[endIndex]->tick() + relativeEnd;
      ChordRest* first = score->findCR(measures[index]->tick() + relativeTick, track);
      if (!first || end < first->tick())
            return;
      const bool melisma = properties.firstChildElement("isEndMelisma").text().toInt() != 0;
      const int verse = properties.firstChildElement("no").text().toInt();
      Placement placement = score->styleV(Sid::lyricsPlacement).value<Placement>();
      const QString position = properties.firstChildElement("placement").text();
      if (!position.isEmpty())
            placement = position == "above" ? Placement::ABOVE : Placement::BELOW;
      ChordRest* endCR = score->findCR(end, track);
      const Lyrics* endLyrics = endCR ? endCR->lyrics(verse, placement) : nullptr;
      const bool endsAtSyllable = endLyrics && !endLyrics->plainText().isEmpty();
      ChordRest* last = endChordRest(first, end, melisma || endsAtSyllable);
      Lyrics* owner = first->lyrics(verse, placement);
      if (!owner) {
            owner = new Lyrics(score);
            owner->setTrack(track);
            owner->setNo(verse);
            owner->setPlacement(placement);
            if (!position.isEmpty())
                  owner->setPropertyFlags(Pid::PLACEMENT, PropertyFlags::UNSTYLED);
            first->add(owner);
            }
      const bool outgoing = end == last->measure()->endTick() && hasFollowingRepeatBoundary(last->measure());
      owner->setProperty(Pid::PARTIAL_SPANNER_DIRECTION, outgoing || owner->isOutgoing() ? 2 : 0);
      owner->setProperty(Pid::PARTIAL_LYRIC_MELISMA, melisma);
      owner->setTicks(last->tick() - first->tick());
      }

void importStudioRepeatLyrics(MasterScore* score, const QByteArray& data)
      {
      if (score->mscVersion() < 400 || score->mscVersion() >= 500)
            return;
      if (!data.contains("<Lyrics") && !data.contains("PartialLyricsLine"))
            return;
      QDomDocument document;
      if (!document.setContent(data))
            return;
      const QDomElement xmlScore = document.documentElement().firstChildElement("Score");
      std::vector<Measure*> measures;
      for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure())
            measures.push_back(m);
      if (measures.empty())
            return;
      // Reading elements may have queried the roadmap before the score was
      // complete. Resolve repeat destinations against the finished score.
      score->setPlaylistDirty();
      for (QDomElement staff = xmlScore.firstChildElement("Staff"); !staff.isNull(); staff = staff.nextSiblingElement("Staff")) {
            const int staffIndex = staff.attribute("id").toInt() - 1;
            if (staffIndex < 0 || staffIndex >= score->nstaves())
                  continue;
            int index = 0;
            for (QDomElement measure = staff.firstChildElement("Measure"); !measure.isNull() && index < int(measures.size());
                 measure = measure.nextSiblingElement("Measure"), ++index) {
                  int voiceIndex = 0;
                  for (QDomElement voice = measure.firstChildElement("voice"); !voice.isNull() && voiceIndex < VOICES;
                       voice = voice.nextSiblingElement("voice"), ++voiceIndex) {
                        const int track = staffIndex * VOICES + voiceIndex;
                        Fraction tick(0, 1);
                        for (QDomElement item = voice.firstChildElement(); !item.isNull(); item = item.nextSiblingElement()) {
                              if (item.tagName() == "location") {
                                    const QString fraction = item.firstChildElement("fractions").text();
                                    if (!fraction.isEmpty())
                                          tick += Fraction::fromString(fraction);
                                    }
                              else if (item.tagName() == "Spanner" && item.attribute("type") == "PartialLyricsLine")
                                    importIncomingLine(score, measures, index, track, tick, item);
                              else if ((item.tagName() == "Chord" && !isGrace(item)) || item.tagName() == "Rest") {
                                    ChordRest* cr = score->findCR(measures[index]->tick() + tick, track);
                                    if (cr)
                                          tick = cr->tick() + cr->actualTicks() - measures[index]->tick();
                                    }
                              }
                        }
                  }
            }

      // Studio represents an outgoing repeat melisma with ordinary Lyrics
      // (often the temporary one-tick marker), not a PartialLyricsLine record.
      for (Measure* measure : measures) {
            if (!hasFollowingRepeatBoundary(measure))
                  continue;
            for (Segment* seg = measure->first(SegmentType::ChordRest); seg; seg = seg->next(SegmentType::ChordRest)) {
                  for (int track = 0; track < score->ntracks(); ++track) {
                        Element* item = seg->element(track);
                        if (!item || !item->isChordRest())
                              continue;
                        ChordRest* cr = toChordRest(item);
                        for (Lyrics* lyric : cr->lyrics()) {
                              if (lyric->isPartial())
                                    continue;
                              const bool melisma = lyric->ticks().isNotZero();
                              if (!melisma && lyric->syllabic() != Lyrics::Syllabic::BEGIN && lyric->syllabic() != Lyrics::Syllabic::MIDDLE)
                                    continue;
                              const bool temporary = lyric->ticks() == Fraction::fromTicks(Lyrics::TEMP_MELISMA_TICKS);
                              ChordRest* last = cr;
                              bool nextSyllable = false;
                              for (Segment* s = seg->next(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
                                    Element* next = s->element(track);
                                    if (next && next->isChordRest()) {
                                          last = toChordRest(next);
                                          Lyrics* l = last->lyrics(lyric->no(), lyric->placement());
                                          if (l && !l->plainText().isEmpty())
                                                nextSyllable = true;
                                          }
                                    }
                              if (melisma) {
                                    ChordRest* end = temporary ? cr : score->findCR(lyric->endTick(), track);
                                    if (end != last)
                                          continue;
                                    }
                              else if (nextSyllable)
                                    continue;
                              if (last->tick() + last->actualTicks() != measure->endTick())
                                    continue;
                              lyric->setProperty(Pid::PARTIAL_SPANNER_DIRECTION, 1);
                              lyric->setProperty(Pid::PARTIAL_LYRIC_MELISMA, melisma);
                              lyric->setTicks(last->tick() - cr->tick());
                              }
                        }
                  }
            }
      }
}
