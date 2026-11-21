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
            fullMeasureRestUsesLocalClearance
            fullMeasureRestClearsAtItsOffset
            fullMeasureRestKeepsClearanceOnLookahead
      )
set_tests_properties(tst_incrementallayout_rest_preview PROPERTIES
      ENVIRONMENT "MSCORE_NATIVE_REST_LAYOUT=1"
      LABELS "native-layout-preview"
      )

set(TARGET tst_benchmark)

include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)
