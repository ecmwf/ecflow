# avoid warning about dependency targets that don't exit
#
# CMake Warning (dev) at Viewer/src/CMakeLists.txt:261 (ADD_DEPENDENCIES):
#  Policy CMP0046 is not set: Error on non-existent dependency in
#  add_dependencies.  Run "cmake --help-policy CMP0046" for policy details.
#  Use the cmake_policy command to set the policy and suppress this warning.
#
#  The dependency target
#  "/tmp/ma0/workspace/ecflow/Viewer/src/../images/zombie_dock.svg" of target
#  "Qt_resource_cpp" does not exist.

cmake_policy(SET CMP0046 OLD)

add_subdirectory( src )
