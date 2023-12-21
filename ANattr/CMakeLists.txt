#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  src/ecflow/attribute/AutoArchiveAttr.hpp
  src/ecflow/attribute/AutoCancelAttr.hpp
  src/ecflow/attribute/ClockAttr.hpp
  src/ecflow/attribute/CronAttr.hpp
  src/ecflow/attribute/DateAttr.hpp
  src/ecflow/attribute/DayAttr.hpp
  src/ecflow/attribute/GenericAttr.hpp
  src/ecflow/attribute/LateAttr.hpp
  src/ecflow/attribute/NodeAttr.hpp
  src/ecflow/attribute/QueueAttr.hpp
  src/ecflow/attribute/RepeatAttr.hpp
  src/ecflow/attribute/TimeAttr.hpp
  src/ecflow/attribute/TodayAttr.hpp
  src/ecflow/attribute/Variable.hpp
  src/ecflow/attribute/VerifyAttr.hpp
  src/ecflow/attribute/Zombie.hpp
  src/ecflow/attribute/ZombieAttr.hpp
  # Sources
  src/ecflow/attribute/AutoArchiveAttr.cpp
  src/ecflow/attribute/AutoCancelAttr.cpp
  src/ecflow/attribute/ClockAttr.cpp
  src/ecflow/attribute/CronAttr.cpp
  src/ecflow/attribute/DateAttr.cpp
  src/ecflow/attribute/DayAttr.cpp
  src/ecflow/attribute/GenericAttr.cpp
  src/ecflow/attribute/LateAttr.cpp
  src/ecflow/attribute/NodeAttr.cpp
  src/ecflow/attribute/QueueAttr.cpp
  src/ecflow/attribute/RepeatAttr.cpp
  src/ecflow/attribute/TimeAttr.cpp
  src/ecflow/attribute/TodayAttr.cpp
  src/ecflow/attribute/Variable.cpp
  src/ecflow/attribute/VerifyAttr.cpp
  src/ecflow/attribute/Zombie.cpp
  src/ecflow/attribute/ZombieAttr.cpp
)

ecbuild_add_library(
  TARGET
    attributes
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    core
    Boost::date_time
)
target_clangformat(attributes)


set(test_srcs
  # Sources
  test/TestAttributes_main.cpp
  test/TestAttrSerialization.cpp
  test/TestCron.cpp
  test/TestDateAttr.cpp
  test/TestDayAttr.cpp
  test/TestLabel.cpp
  test/TestLateAttr.cpp
  test/TestMigration.cpp
  test/TestRepeat.cpp
  test/TestSizeOf.cpp
  test/TestTimeAttr.cpp
  test/TestTodayAttr.cpp
  test/TestVariable.cpp
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
    test_support
    attributes
  TEST_DEPENDS
    u_core
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_attributes
  CONDITION ENABLE_TESTS
)
