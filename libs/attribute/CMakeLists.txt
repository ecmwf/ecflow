#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

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

ecbuild_add_test(
  TARGET
    u_attributes
  LABELS
    unit
    nightly
  SOURCES
    ${test_srcs}
  LIBS
    ecflow_all
    test_scaffold
  TEST_DEPENDS
    u_core
)
target_clangformat(u_attributes
  CONDITION ENABLE_TESTS
)
