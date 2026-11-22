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

add_test(NAME tst_spanners_legacy_slurs
      COMMAND tst_spanners -xunitxml -o result-legacy-slurs.xml
            nativeMultiVoiceSlurDirection
      )
set_tests_properties(tst_spanners_legacy_slurs PROPERTIES
      ENVIRONMENT "MSCORE_LEGACY_SLUR_FIT=1"
      )
