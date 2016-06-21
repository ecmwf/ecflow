### ecflow python bindings
# NOTES:
#
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
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )

# =====================================================================
# local includes
include_directories(  
                     ../ACore/src 
                     ../ANattr/src
                     ../ANode/src
                     ../AParser/src
                     ../Base/src
                     ../Base/src/cts
                     ../Base/src/stc
                     ../Client/src
                     ../CSim/src
                    )

# =====================================================================     
# INCLUDES here is for external includes   
#   NOINSTALL this stops double install, the one in i.e lib/ecflow.so
#             The other install is in python2.7/site-packages/ecflow.so
ecbuild_add_library( TARGET    ecflow
                     NOINSTALL
                     TYPE      MODULE
                     SOURCES   ${srcs}
					 CONDITION PYTHONLIBS_FOUND
                     LIBS      libclient libsimu base libparser node nodeattr core
                               ${PYTHON_LIBRARIES}
                     INCLUDES 
                               ${Boost_INCLUDE_DIRS}
                               ${PYTHON_INCLUDE_DIRS} 
                    )

# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs 
if (   PYTHONLIBS_FOUND )             
   target_link_libraries(ecflow debug      ${Boost_PYTHON_LIBRARY_DEBUG}   )
   target_link_libraries(ecflow optimized  ${Boost_PYTHON_LIBRARY_RELEASE} )
endif()

#
# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#         
# by default cmake add prefix 'lib', we don't want this hence disable
set_target_properties(ecflow PROPERTIES 
                             PREFIX ""
                             INSTALL_RPATH ""
                     )
                     
# =====================================================================
# tests 

ecbuild_add_test( TARGET py_u_TestMigrate
                  TYPE PYTHON 
                  ARGS ${CMAKE_CURRENT_SOURCE_DIR}/migrate/py_u_TestMigrate.py
                  ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/Pyext" )

list( APPEND u_tests
        py_u_TestAddDelete   
        py_u_TestAddDeleteFunc 
        py_u_TestAddNodeFunc  
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
        py_u_TestRepeatArithmetic
        py_u_TestSimulator  
        py_u_TestTraversal     
        py_u_TestUserManual    
        py_u_TestWith         
)
foreach( test ${u_tests} )

    ecbuild_add_test( TARGET ${test} 
                      TYPE PYTHON 
                      ARGS        ${CMAKE_CURRENT_SOURCE_DIR}/test/${test}.py
                      ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/Pyext" 
                      TEST_DEPENDS u_base
                     )
endforeach()


if ( ENABLE_ALL_TESTS)
	list( APPEND s_tests
	             py_s_TestClientApi    
	             py_s_TestPythonChildApi
	)
	
	foreach( test ${s_tests} )
	
	    ecbuild_add_test( TARGET ${test} 
	                      TYPE PYTHON 
	                      ARGS        ${CMAKE_CURRENT_SOURCE_DIR}/test/${test}.py
	                      ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/Pyext" 
	                      TEST_DEPENDS u_base
	                     )
	endforeach()
	
	set_property(TEST py_s_TestClientApi      APPEND PROPERTY DEPENDS s_test)
	set_property(TEST py_s_TestPythonChildApi APPEND PROPERTY DEPENDS py_s_TestClientApi)
endif()


# ==========================================================================
# install
#    -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup | not defined ]
#
#    local | not defined : this will install to:
#                          $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup               : experimental only,python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory
#
# Note:  To install only the python module
#       cd  buildir
#       cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCOMPONENT=python -P cmake_install.cmake -- make install
# ==========================================================================
   
if( CMAKE_PYTHON_INSTALL_TYPE MATCHES  "local"  OR NOT DEFINED CMAKE_PYTHON_INSTALL_TYPE )
   
      message(STATUS "python install *LOCAL* +++ lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages/ecflow +++++++")
      set(PYTHON_SITE "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages" )
      set(PYTHON_DEST "${PYTHON_SITE}/ecflow" )
      
      install( TARGETS ecflow              DESTINATION ${PYTHON_DEST}  RENAME ecflow.so COMPONENT python )
      install( FILES   ecflow/__init__.py  DESTINATION ${PYTHON_DEST}                   COMPONENT python )
      
else()
	  message( STATUS "python found,  CMAKE_PYTHON_INSTALL_TYPE=${CMAKE_PYTHON_INSTALL_TYPE}")

      # -------------------------------------------------------------------------------------
      # Install using setup.py
      # See: http://bloerg.net/2012/11/10/cmake-and-distutils.html
      # -------------------------------------------------------------------------------------
      message(STATUS "python install using *setup.py* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
      message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
      message(STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
      message(STATUS "CMAKE_PYTHON_INSTALL_PREFIX : ${CMAKE_PYTHON_INSTALL_PREFIX}" )  
    
      set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
      set(SETUP_PY    "${CMAKE_CURRENT_SOURCE_DIR}/setup.py")
      set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/ecflow/__init__.py")
      set(OUTPUT      "${CMAKE_CURRENT_SOURCE_DIR}/timestamp")

      configure_file(${SETUP_PY_IN} ${SETUP_PY} )

      add_custom_command(OUTPUT ${OUTPUT}
                       COMMAND ${PYTHON} ${SETUP_PY} build
                       COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
                       DEPENDS ${DEPS})
      add_custom_target(target ALL DEPENDS ${OUTPUT})


      install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} build_ext)")
    
	  if( DEFINED CMAKE_PYTHON_INSTALL_PREFIX )
         message(STATUS "custom/*test* python install prefix defined CMAKE_PYTHON_INSTALL_PREFIX=${CMAKE_PYTHON_INSTALL_PREFIX}")
         install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install -f --prefix=${CMAKE_PYTHON_INSTALL_PREFIX})")
      else()
         install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
      endif()
endif()
