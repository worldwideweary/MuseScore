//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __SPANNERMAP_H__
#define __SPANNERMAP_H__


#include "thirdparty/intervaltree/IntervalTree.h"
#include <unordered_map>

namespace Ms {

class Spanner;

//---------------------------------------------------------
//   SpannerMap
//---------------------------------------------------------

class SpannerMap : std::multimap<int, Spanner*> {
      mutable bool dirty;
      mutable bool mapOrderDirty = true;
      std::unordered_map<const Spanner*, std::size_t> mapOrder;
      mutable interval_tree::IntervalTree<Spanner*> tree;
      std::vector< interval_tree::Interval<Spanner*> > results;

   public:
      SpannerMap();
      const std::vector< interval_tree::Interval<Spanner*> >& findContained(int start, int stop);
      const std::vector< interval_tree::Interval<Spanner*> >& findOverlapping(int start, int stop);
      // Native map iteration order, including equivalent-key insertion order.
      // Rank data is lazy and invalidated with the existing interval index;
      // ordinary callers do not build or scan it.
      std::vector<Spanner*> findOverlappingInMapOrder(int start, int stop);
      const std::multimap<int, Spanner*>& map() const { return *this; }
      std::multimap<int,Spanner*>::const_reverse_iterator crbegin() const { return std::multimap<int, Spanner*>::crbegin(); }
      std::multimap<int,Spanner*>::const_reverse_iterator crend() const   { return std::multimap<int, Spanner*>::crend(); }
      std::multimap<int,Spanner*>::const_iterator cbegin() const { return std::multimap<int, Spanner*>::cbegin(); }
      std::multimap<int,Spanner*>::const_iterator cend() const  { return std::multimap<int, Spanner*>::cend(); }
      void addSpanner(Spanner* s);
      bool removeSpanner(Spanner* s);
      void clear() { std::multimap<int, Spanner*>::clear(); mapOrder.clear(); setDirty(); }
      void update() const;
      void setDirty() const { dirty = true; mapOrderDirty = true; }   // must be called if a spanner changes start/length
#ifndef NDEBUG
      void dump() const;
#endif
      };

}     // namespace Ms

#endif

