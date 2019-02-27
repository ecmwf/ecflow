# =====================================================================     
# INCLUDES here is for external includes   
#   NOINSTALL this stops double install, the one in i.e lib/ecflow.so
#             The other install is in python2.7/site-packages/ecflow.so
# if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "../src/*.cpp" "../src/*.hpp" )
ecbuild_add_library( TARGET    ecflow3
                     NOINSTALL
                     TYPE      MODULE
                     SOURCES   ${srcs}
                     LIBS      libclient libsimu base node nodeattr core
                               ${Python3_LIBRARIES} ${OPENSSL_LIBRARIES}
                     INCLUDES 
                               ${Boost_INCLUDE_DIRS}
                               ${Python3_INCLUDE_DIRS}
                    )


# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs 
# Currently we only build optimised boost libs
# versions of boost >= 1.67 now tag the python libs, i.e. libboost_python27-mt.a, libboost_python36-mt.so
if ( Boost_MINOR_VERSION GREATER 66 )
    target_link_libraries(ecflow3 ${Boost_PYTHON${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_LIBRARY_RELEASE} )
else()
    target_link_libraries(ecflow3 ${Boost_PYTHON3_LIBRARY_RELEASE} )
endif()

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#         
# by default cmake add prefix 'lib', we don't want this hence disable
#
# To avoid duplicate target names with python2 we chose ecflow3, however test and user code
# depend on name 'ecflow' hence rename using 'OUTPUT_NAME'
set_target_properties(ecflow3 PROPERTIES 
                             OUTPUT_NAME "ecflow"
                             PREFIX ""
                             INSTALL_RPATH ""
                     )
                     
# =====================================================================
# tests 

#ecbuild_add_test( TARGET py3_u_TestMigrate
#                  TYPE PYTHON 
#                  ARGS ${CMAKE_CURRENT_SOURCE_DIR}/../migrate/py_u_TestMigrate.py
#                  ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}" )

list( APPEND u_tests
        u_test_add   
        u_test_collection   
        u_test_cron  
        u_test_defs_constructor 
        u_test_get_attr  
        u_test_manual   
        u_test_late   
        u_test_replace_node   
        u_test_tutorial   
        u_TestAddDelete   
        u_TestAddDeleteFunc 
        u_TestAddNodeFunc  
        u_TestAutoAddExtern  
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
        u_TestParent        
        u_TestRemove
        u_TestRepeatArithmetic
        u_TestSimulator  
        u_TestTraversal     
        u_TestUserManual    
        u_TestWith         
)
foreach( test ${u_tests} )

    ecbuild_add_test( TARGET py3_${test} 
                      TYPE PYTHON 
                      ARGS        ${CMAKE_CURRENT_SOURCE_DIR}/../test/py_${test}.py
                      ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}" 
                      TEST_DEPENDS u_base
                     )
endforeach()


if ( ENABLE_ALL_TESTS AND ENABLE_SERVER)
	list( APPEND s_tests
	             s_TestClientApi    
	             s_TestPythonChildApi
	)
	
	foreach( test ${s_tests} )
	
	    ecbuild_add_test( TARGET py3_${test} 
	                      TYPE PYTHON 
	                      ARGS        ${CMAKE_CURRENT_SOURCE_DIR}/../test/py_${test}.py
	                      ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}" 
	                      TEST_DEPENDS u_base
	                     )
	endforeach()
	
	set_property(TEST py3_s_TestClientApi      APPEND PROPERTY DEPENDS s_test)
	set_property(TEST py3_s_TestPythonChildApi APPEND PROPERTY DEPENDS py3_s_TestClientApi)
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
   
      message(STATUS "python install *LOCAL* +++ lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/ecflow +++++++")
      set(PYTHON_SITE "lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages" )
      set(PYTHON_DEST "${PYTHON_SITE}/ecflow" )
      
      install( TARGETS ecflow3                DESTINATION ${PYTHON_DEST}  RENAME ecflow.so COMPONENT python )
      install( FILES   ../ecflow/__init__.py  DESTINATION ${PYTHON_DEST}                   COMPONENT python )
      
      # Install files that Axel has asked for. 
      # These are example of what Axel uses as a layer over ecflow api, for operations
      install( FILES   ../samples/api/ecf.py     DESTINATION ${PYTHON_DEST}                COMPONENT python )
      install( FILES   ../samples/api/sms2ecf.py DESTINATION ${PYTHON_DEST}                COMPONENT python )
      
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
    
      set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/../setup.py.in")
      set(SETUP_PY    "${CMAKE_CURRENT_SOURCE_DIR}/../setup.py")
      set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/../ecflow/__init__.py")
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
