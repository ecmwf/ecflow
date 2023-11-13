#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# avoid warning about dependency targets that don't exist
#
# CMake Warning (dev) at Viewer/src/CMakeLists.txt:261 (ADD_DEPENDENCIES):
#  Policy CMP0046 is not set: Error on non-existent dependency in
#  add_dependencies.  Run "cmake --help-policy CMP0046" for policy details.
#  Use the cmake_policy command to set the policy and suppress this warning.
#
#  The dependency target
#  "/tmp/ma0/workspace/ecflow/Viewer/src/../images/zombie_dock.svg" of target
#  "Qt_resource_cpp" does not exist.

if (POLICY CMP0046)
  cmake_policy(SET CMP0046 OLD)
endif()

add_subdirectory( libViewer)
add_subdirectory( ecflowUI)
