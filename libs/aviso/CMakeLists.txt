#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# libaviso
# --------------------------------------------------

set(srcs
  # Headers
  src/aviso/Aviso.hpp
  # Sources
  src/aviso/Aviso.cpp
)

ecbuild_add_library(
  TARGET libaviso
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    Boost::boost
    nlohmann::json
    httplib::httplib
)
target_clangformat(libaviso)

# ecflow_aviso
# --------------------------------------------------

ecbuild_add_executable(
  TARGET ecflow_aviso
  SOURCES
    src/aviso/AvisoMain.cpp
  LIBS
    libaviso
)
target_clangformat(ecflow_aviso)

# u_aviso
# --------------------------------------------------

set(test_srcs
  # Sources
  test/TestAviso.cpp
  test/TestAviso_main.cpp # test entry point
)

ecbuild_add_test(
  TARGET
    u_aviso
  LABELS
    integration
  SOURCES
    ${test_srcs}
  LIBS
    libaviso
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_aviso
  CONDITION ENABLE_TESTS
)
