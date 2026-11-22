#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2011-2015 Werner Schweer & others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

set(TARGET tst_spanners)

include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)

# Run the preview cases in a separate process; keep the normal suite's
# engraving and reference scores on the default path.
add_test(NAME tst_spanners_slur_preview
      COMMAND tst_spanners -xunitxml -o result-slur-preview.xml
            nativeMultiVoiceSlurDirection
            nativeOrdinarySlurFit
            nativeOrdinarySlurAcrossBarline
            nativeArticulatedSlurTips
            nativeSlantedSlurFit
            nativeAfterGraceSlurFit
            nativePreGraceSlurFit
            nativeNestedBeamedSlurFit
            nativeCrossStaffSlurs
            nativeReturningCrossStaffSlurs
            nativeSlursOverTies
            nativeSlurOverInteriorTie
            nativeSystemBreakSlur
            nativeContinuedSlurCurve
      )
set_tests_properties(tst_spanners_slur_preview PROPERTIES
      ENVIRONMENT "MSCORE_NATIVE_SLUR_FIT=1"
      LABELS "native-layout-preview"
      )
