### ecflow python bindings

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )

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
      
# INCLUDES here is for external includes           
ecbuild_add_library( TARGET   ecflow
                     TYPE     MODULE
                     SOURCES  ${srcs}
					 CONDITION PYTHONLIBS_FOUND
                     LIBS     libclient libsimu base libparser node nodeattr core
                              ${Boost_PYTHON_LIBRARY} 
                              ${PYTHON_LIBRARIES}
                     INCLUDES 
                            ${Boost_INCLUDE_DIRS}
                            ${PYTHON_INCLUDE_DIRS} 
                    )

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

### tests 

list( APPEND tests
        py_u_TestAddDelete   
        py_u_TestAddDeleteFunc 
        py_u_TestAddNodeFunc  
        py_s_TestClientApi    
        py_s_TestPythonChildApi
        py_u_TestDefs         
        py_u_TestDefsCheck    
        py_u_TestDerivable   
        py_u_TestEcf  
        py_u_TestError        
        py_u_TestFind        
        py_u_TestGetAllTasks   
        py_u_TestJobGeneration 
        py_u_TestParent        
        py_u_TestRepeatArithmetic
        py_u_TestSimulator  
        py_u_TestTraversal     
        py_u_TestUserManual    
        py_u_TestWith         
)

foreach( test ${tests} )

    ecbuild_add_test( TARGET ${test} 
                      TYPE PYTHON 
                      ARGS ${CMAKE_CURRENT_SOURCE_DIR}/test/${test}.py
                      ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/lib" )
endforeach()


ecbuild_add_test( TARGET py_u_TestMigrate
                  TYPE PYTHON 
                  ARGS ${CMAKE_CURRENT_SOURCE_DIR}/migrate/py_u_TestMigrate.py
                  ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/lib" )


# ==========================================================================
# install
# ==========================================================================
if( ENABLE_PYTHON )

   message( STATUS "python found,  CMAKE_PYTHON_INSTALL_TYPE=${CMAKE_PYTHON_INSTALL_TYPE}")

   # CMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
   
   if( CMAKE_PYTHON_INSTALL_TYPE MATCHES  "local"  OR NOT DEFINED CMAKE_PYTHON_INSTALL_TYPE )
   
      message(STATUS "python install *LOCAL* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
      set(PYTHON_SITE "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages" )
      set(PYTHON_DEST "${PYTHON_SITE}/ecflow" )
      
      install( TARGETS ecflow   DESTINATION ${PYTHON_DEST} 
               RENAME ecflow.so
             )
      install( FILES   ecflow/__init__.py  DESTINATION ${PYTHON_DEST} )
      
   else()

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
      set(OUTPUT      "${CMAKE_CURRENT_SOURCE_DIR}/build/timestamp")

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

endif()
