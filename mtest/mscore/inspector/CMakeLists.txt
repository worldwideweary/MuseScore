set(TARGET tst_repeatinspector)
set(MTEST_LINK_MSCOREAPP TRUE)
include_directories(${PROJECT_BINARY_DIR}/mscore ${PROJECT_SOURCE_DIR}/mscore)
include(${PROJECT_SOURCE_DIR}/mtest/CreateMtestTarget.cmake)
