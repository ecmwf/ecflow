# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set(test_srcs
  # Sources
  test/TestAttributes_main.cpp
  test/TestAttrSerialization.cpp
  test/TestAvisoAttr.cpp
  test/TestCron.cpp
  test/TestDateAttr.cpp
  test/TestDayAttr.cpp
  test/TestLabel.cpp
  test/TestLateAttr.cpp
  test/TestMigration.cpp
  test/TestRepeat.cpp
  test/TestRepeatRange.cpp
  test/TestSizeOf.cpp
  test/TestTimeAttr.cpp
  test/TestTodayAttr.cpp
  test/TestVariable.cpp
  test/TestVariableMap.cpp
  test/TestZombieAttr.cpp
)

ecflow_add_test(
  TARGET
    u_attributes
  LABELS
    unit nightly fast
  SOURCES
    ${test_srcs}
  LIBS
    ecflow_all
    test_scaffold
  TEST_DEPENDS
    u_core
  CONDITION
    ENABLE_TESTS
  FORMAT
)
