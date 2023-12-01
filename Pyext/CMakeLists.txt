#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

### ecflow python bindings
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
# source files (paths relative to dirs python2/python3)
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
  py_u_test_add
  py_u_test_collection
  py_u_test_cron
  py_u_test_defs_constructor
  py_u_test_get_attr
  py_u_test_manual
  py_u_test_late
  py_u_test_replace_node
  py_u_test_tutorial
  py_u_TestAddDelete
  py_u_TestAddDeleteFunc
  py_u_TestAddNodeFunc
  py_u_TestAutoAddExtern
  py_u_TestCopy
  py_u_TestDefs
  py_u_TestDefsCheck
  py_u_TestDerivable
  py_u_TestEcf
  py_u_TestError
  py_u_TestFind
  py_u_TestFlag
  py_u_TestGetAllTasks
  py_u_TestJobGeneration
  py_u_TestParent
  py_u_TestRemove
  py_u_TestRepeatArithmetic
  py_u_TestSimulator
  py_u_TestTraversal
  py_u_TestUserManual
  py_u_TestWith
  py_u_sort
)

set(s_tests
  s_TestClientApi
  s_TestPythonChildApi
)


message( STATUS "====================================================================================================================" )
message( STATUS "PYTHON2" )
message( STATUS "====================================================================================================================" )
if(${CMAKE_VERSION} VERSION_LESS "3.12.0") 
  # We only support python2 extension for cmake less 3.12.0
  # cmake 3.12.0 or greater allows multiple boost python libs & hence multiple extensions to be built
  # Using -DPYTHON_EXECUTABLE=/usr/local/apps/python3/3.6.5-01/bin/python3 is not sufficient as the wrong(python2) libs are found
  ecbuild_find_python( VERSION 2.6 REQUIRED )  
  message( STATUS "  PYTHON_FOUND                      : ${PYTHON_FOUND}" )
  message( STATUS "  PYTHONINTERP_FOUND                : ${PYTHONINTERP_FOUND}" )
  message( STATUS "  PYTHONLIBS_FOUND                  : ${PYTHONLIBS_FOUND}" )
  message( STATUS "  PYTHON_VERSION_STRING             : ${PYTHON_VERSION_STRING}" )
  message( STATUS "  PYTHON_VERSION_MAJOR              : ${PYTHON_VERSION_MAJOR}" )
  message( STATUS "  PYTHON_VERSION_MINOR              : ${PYTHON_VERSION_MINOR}" )
  message( STATUS "  PYTHON_VERSION_PATCH              : ${PYTHON_VERSION_PATCH}" )
  message( STATUS "  PYTHON_CONFIG_EXECUTABLE          : ${PYTHON_CONFIG_EXECUTABLE}" )
  message( STATUS "  PYTHON_EXECUTABLE                 : ${PYTHON_EXECUTABLE}" )
  message( STATUS "  PYTHON_INCLUDE_DIRS               : ${PYTHON_INCLUDE_DIRS}" )
  message( STATUS "  PYTHON_LIBRARIES                  : ${PYTHON_LIBRARIES}" )
  if (${PYTHON_VERSION_MAJOR} EQUAL 3)
    ecbuild_error("Need cmake version >= 3.12.0 to build ecflow python3 extension, current cmake version is ${CMAKE_VERSION}")
  endif()

  find_package( Boost ${ECFLOW_BOOST_VERSION} REQUIRED COMPONENTS python )
  add_subdirectory( python2 )
else()
  # ======================================================================================
  # Attempt to build both python2 *AND/OR* python3 ecflow extension, depending on what is found
  # this assumes cmake 3.12.0 min, which added support for boost python2 and python3
  # * NOTICE* that we do *NOT* use REQUIRED when searching for boost python libs
  # ======================================================================================
  find_package(Python2 COMPONENTS Interpreter Development)
  if (Python2_FOUND)
    message( STATUS "  Python2_Interpreter_FOUND         : ${Python2_Interpreter_FOUND}" )
    message( STATUS "  Python2_EXECUTABLE                : ${Python2_EXECUTABLE}" )
    message( STATUS "  Python2_STDLIB                    : ${Python2_STDLIB}    Standard platform independent installation directory" )
    message( STATUS "  Python2_STDARCH                   : ${Python2_STDARCH}   Standard platform dependent installation directory." )
    message( STATUS "  Python2_Development_FOUND         : ${Python2_Development_FOUND}" )
    message( STATUS "  Python2_INCLUDE_DIRS              : ${Python2_INCLUDE_DIRS}" )
    message( STATUS "  Python2_LIBRARIES                 : ${Python2_LIBRARIES}" )
    message( STATUS "  Python2_LIBRARY_DIRS              : ${Python2_LIBRARY_DIRS}" )
    message( STATUS "  Python2_VERSION                   : ${Python2_VERSION}" )
    message( STATUS "  Python2_VERSION_MAJOR             : ${Python2_VERSION_MAJOR}" )
    message( STATUS "  Python2_VERSION_MINOR             : ${Python2_VERSION_MINOR}" )
    message( STATUS "  Python2_VERSION_PATCH             : ${Python2_VERSION_PATCH}" )
    # *****************************************************************************************
    # Although we have found python2 it could be that *ONLY* python3 was module loaded
    # Otherwise will build for python2 and test with python3 interpreter
    # *****************************************************************************************
    if ( Python2_LIBRARIES )
      ecbuild_find_python( VERSION 2.6 REQUIRED ) # if not included iterpreter not found ?
      if ( ${PYTHON_VERSION_MAJOR} EQUAL 2)
        message( STATUS "  PYTHON_FOUND                      : ${PYTHON_FOUND}" )
        message( STATUS "  PYTHONINTERP_FOUND                : ${PYTHONINTERP_FOUND}" )
        message( STATUS "  PYTHONLIBS_FOUND                  : ${PYTHONLIBS_FOUND}" )
        message( STATUS "  PYTHON_VERSION_STRING             : ${PYTHON_VERSION_STRING}" )
        message( STATUS "  PYTHON_VERSION_MAJOR              : ${PYTHON_VERSION_MAJOR}" )
        message( STATUS "  PYTHON_VERSION_MINOR              : ${PYTHON_VERSION_MINOR}" )
        message( STATUS "  PYTHON_VERSION_PATCH              : ${PYTHON_VERSION_PATCH}" )
        message( STATUS "  PYTHON_CONFIG_EXECUTABLE          : ${PYTHON_CONFIG_EXECUTABLE}" )
        message( STATUS "  PYTHON_EXECUTABLE                 : ${PYTHON_EXECUTABLE}" )
        message( STATUS "  PYTHON_INCLUDE_DIRS               : ${PYTHON_INCLUDE_DIRS}" )
        message( STATUS "  PYTHON_LIBRARIES                  : ${PYTHON_LIBRARIES}" )

        if ( Boost_MINOR_VERSION GREATER 66 )
          # cmake 3.15
          # see: https://gitlab.kitware.com/cmake/cmake/issues/19656
          # INTERFACE_LIBRARY targets may only have whitelisted properties."
          set(ECFLOW_BOOST_PYTHON_COMPONENT python${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR})
        else()
          set(ECFLOW_BOOST_PYTHON_COMPONENT python)
        endif()
        find_package( Boost ${ECFLOW_BOOST_VERSION} COMPONENTS ${ECFLOW_BOOST_PYTHON_COMPONENT} )

        if (Boost_PYTHON_FOUND OR Boost_PYTHON${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}_FOUND)
          set(ECF_PYTHON_FOUND "ECF_PYTHON_FOUND")
          add_subdirectory( python2 )
        else()
          message( STATUS "  Boost python2 libraries *NOT* found" )
        endif()
      endif()
    else()
      message( STATUS "  Python2 libraries *NOT* found" )
    endif()
  endif()

  message( STATUS "====================================================================================================================" )
  message( STATUS "PYTHON3" )
  message( STATUS "====================================================================================================================" )
  # The python must include the Development packages. As the headers in these packages is used by boost python.
  find_package(Python3 COMPONENTS Interpreter Development)
  if (Python3_FOUND)
    message( STATUS "  Python3_Interpreter_FOUND         : ${Python3_Interpreter_FOUND}" )
    message( STATUS "  Python3_EXECUTABLE                : ${Python3_EXECUTABLE}" )
    message( STATUS "  Python3_STDLIB                    : ${Python3_STDLIB}    Standard platform independent installation directory" )
    message( STATUS "  Python3_STDARCH                   : ${Python3_STDARCH}   Standard platform dependent installation directory." )
    message( STATUS "  Python3_Development_FOUND         : ${Python3_Development_FOUND}" )
    message( STATUS "  Python3_INCLUDE_DIRS              : ${Python3_INCLUDE_DIRS}" )
    message( STATUS "  Python3_LIBRARIES                 : ${Python3_LIBRARIES}" )
    message( STATUS "  Python3_LIBRARY_DIRS              : ${Python3_LIBRARY_DIRS}" )
    message( STATUS "  Python3_VERSION                   : ${Python3_VERSION}" )
    message( STATUS "  Python3_VERSION_MAJOR             : ${Python3_VERSION_MAJOR}" )
    message( STATUS "  Python3_VERSION_MINOR             : ${Python3_VERSION_MINOR}" )
    message( STATUS "  Python3_VERSION_PATCH             : ${Python3_VERSION_PATCH}" )

    if ( Boost_MINOR_VERSION GREATER 66 )
      # cmake 3.15
      # see: https://gitlab.kitware.com/cmake/cmake/issues/19656
      # INTERFACE_LIBRARY targets may only have whitelisted properties.
      set(ECFLOW_BOOST_PYTHON_COMPONENT python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})
    else()
      set(ECFLOW_BOOST_PYTHON_COMPONENT python3)
    endif()
    find_package( Boost ${ECFLOW_BOOST_VERSION} COMPONENTS ${ECFLOW_BOOST_PYTHON_COMPONENT} )
    
    if (Boost_PYTHON3_FOUND OR Boost_PYTHON${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_FOUND)
      if ( Python3_LIBRARIES )
        ecbuild_find_python( VERSION ${Python3_VERSION} REQUIRED )
        set(ECF_PYTHON_FOUND "ECF_PYTHON_FOUND")
        add_subdirectory( python3 )
      else()
        message( STATUS "  python3 libraries *NOT* found" )
      endif()
    else()
      message( STATUS "  Boost python3 libraries *NOT* found" )
    endif()
  endif()
  
  if (NOT ECF_PYTHON_FOUND)
    ecbuild_error("ecflow python extension is enabled, but python2/python3 libraries or python boost libraries not found")
  endif()
endif()
message( STATUS "====================================================================================================================" )
message( STATUS "END PYTHON" )
message( STATUS "====================================================================================================================" )
