#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2011 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

set(TARGET tst_incrementallayout)
include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)

add_test(NAME tst_incrementallayout_rest_preview
      COMMAND tst_incrementallayout -xunitxml -o result-rest-preview.xml
            pairedWholeRestsUseStaffLines
            mixedRestRunsUseCommonHeight
            movedChordsDoNotDisplaceOrdinaryRests
            halfRestRunsFitSameStaffAndMovedChords
            revealedRestsClearOppositeStems
            fullMeasureRestUsesLocalClearance
            fullMeasureRestClearsAtItsOffset
            fullMeasureRestKeepsClearanceOnLookahead
      )
set_tests_properties(tst_incrementallayout_rest_preview PROPERTIES
      ENVIRONMENT "MSCORE_NATIVE_REST_LAYOUT=1"
      LABELS "native-layout-preview"
      )

add_test(NAME tst_incrementallayout_legacy_stems
      COMMAND tst_incrementallayout -xunitxml -o result-legacy-stems.xml
            crossStaffAutoStemsUseVoiceDirection
            ordinaryStemLengthsUseStudioSteps
      )
set_tests_properties(tst_incrementallayout_legacy_stems PROPERTIES
      ENVIRONMENT "MSCORE_LEGACY_STEM_LAYOUT=1"
      )

set(TARGET tst_benchmark)

include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)
