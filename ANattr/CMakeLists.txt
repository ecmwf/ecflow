#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  src/AutoArchiveAttr.hpp
  src/AutoCancelAttr.hpp
  src/ClockAttr.hpp
  src/CronAttr.hpp
  src/DateAttr.hpp
  src/DayAttr.hpp
  src/GenericAttr.hpp
  src/LateAttr.hpp
  src/NodeAttr.hpp
  src/QueueAttr.hpp
  src/RepeatAttr.hpp
  src/TimeAttr.hpp
  src/TodayAttr.hpp
  src/Variable.hpp
  src/VerifyAttr.hpp
  src/Zombie.hpp
  src/ZombieAttr.hpp
  # Sources
  src/AutoArchiveAttr.cpp
  src/AutoCancelAttr.cpp
  src/ClockAttr.cpp
  src/CronAttr.cpp
  src/DateAttr.cpp
  src/DayAttr.cpp
  src/GenericAttr.cpp
  src/LateAttr.cpp
  src/NodeAttr.cpp
  src/QueueAttr.cpp
  src/RepeatAttr.cpp
  src/TimeAttr.cpp
  src/TodayAttr.cpp
  src/Variable.cpp
  src/VerifyAttr.cpp
  src/Zombie.cpp
  src/ZombieAttr.cpp
)

ecbuild_add_library(
  TARGET
    nodeattr
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
target_clangformat(nodeattr)


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
    u_anattr
  SOURCES
    ${test_srcs}
  LIBS
    nodeattr
  TEST_DEPENDS
    u_acore
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_anattr
  CONDITION ENABLE_TESTS
)
