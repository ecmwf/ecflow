#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

### ecFlow python bindings
#
# NOTES:
# 1/ Building multiple python3 versions not supported, since find_python(..) caches variables, Hack: clear cached variable.
#    https://gitlab.kitware.com/cmake/cmake/issues/19820
#
# 2/
# To see the python link line: Do python-config  --ldflags, i.e
#   > /usr/local/apps/python/2.7.8-01/bin/python-config  --ldflags
#   > -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic
#
# However on cct we get:
#   > /usr/local/apps/python/2.7.5-01/bin/python-config --ldflags
#   > -L/usr/local/apps/python/2.7.5-01/lib/python2.7/config -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic
#
# cct is correct as it has the "-L" but on other machines we rely on /usr/lib/libpython2.7.so.1.0 being there!
# lxop-test does not have /usr/lib/libpython2.7.so.1.0 so ecbuild fails
#
# For problems with the python build, please look in <build-dir>/ecbuild.log, typically non developmental python
# installs may not include python libs or includes
# For ubuntoo to install we need:
#    sudo apt-get install libpython-dev
#

# =============================================================================
# Configure __init__.py to add __version__
# =============================================================================
set(INIT_PY_IN  "${CMAKE_CURRENT_SOURCE_DIR}/ecflow/__init__.py.in")
set(INIT_PY_OUT "${CMAKE_CURRENT_SOURCE_DIR}/ecflow/__init__.py")
configure_file(${INIT_PY_IN} ${INIT_PY_OUT} )


# =============================================================================
# Configure setup.py. Note used locally. Uses existing libraries
# =============================================================================
set(SETUP_PY_IN  "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY_OUT "${CMAKE_CURRENT_SOURCE_DIR}/setup.py")
configure_file(${SETUP_PY_IN} ${SETUP_PY_OUT} )

# ==============================================================================
# source files (paths relative to dirs .../python3)
# ==============================================================================
set(srcs
  # Headers
  ../src/ecflow/python/BoostPythonUtil.hpp
  ../src/ecflow/python/ClientDoc.hpp
  ../src/ecflow/python/DefsDoc.hpp
  ../src/ecflow/python/Edit.hpp
  ../src/ecflow/python/GlossaryDoc.hpp
  ../src/ecflow/python/NodeAttrDoc.hpp
  ../src/ecflow/python/NodeUtil.hpp
  ../src/ecflow/python/PythonBinding.hpp
  ../src/ecflow/python/Trigger.hpp
  # Sources
  ../src/ecflow/python/BoostPythonUtil.cpp
  ../src/ecflow/python/ClientDoc.cpp
  ../src/ecflow/python/DefsDoc.cpp
  ../src/ecflow/python/EcfExt.cpp
  ../src/ecflow/python/Edit.cpp
  ../src/ecflow/python/ExportClient.cpp
  ../src/ecflow/python/ExportCore.cpp
  ../src/ecflow/python/ExportDefs.cpp
  ../src/ecflow/python/ExportNode.cpp
  ../src/ecflow/python/ExportNodeAttr.cpp
  ../src/ecflow/python/ExportSuiteAndFamily.cpp
  ../src/ecflow/python/ExportTask.cpp
  ../src/ecflow/python/GlossaryDoc.cpp
  ../src/ecflow/python/NodeAttrDoc.cpp
  ../src/ecflow/python/NodeUtil.cpp
  ../src/ecflow/python/Trigger.cpp
)

# =====================================================================
# tests
# =====================================================================
set(u_tests
  u_test_add
  u_test_collection
  u_test_cron
  u_test_defs_constructor
  u_test_get_attr
  u_test_manual
  u_test_node
  u_test_late
  u_test_replace_node
  u_test_tutorial
  u_TestAddDelete
  u_TestAddDeleteFunc
  u_TestAddNodeFunc
  u_TestAutoAddExtern
  u_TestAviso
  u_TestCopy
  u_TestDefs
  u_TestDefsCheck
  u_TestDerivable
  u_TestEcf
  u_TestError
  u_TestFind
  u_TestFlag
  u_TestGetAllTasks
  u_TestJobGeneration
  u_TestMirror
  u_TestParent
  u_TestRemove
  u_TestRepeatArithmetic
  u_TestSimulator
  u_TestTraversal
  u_TestUserManual
  u_TestWith
  u_sort
)

set(s_tests
  s_TestClientApi
  s_TestPythonChildApi
  s_TestSslSetup
)

if (Python3_FOUND AND (Boost_PYTHON3_FOUND OR Boost_PYTHON${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_FOUND))
  add_subdirectory( python3 )
endif()
