### ecflow python bindings

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )
#find_package(PythonLibs)
message(${PYTHON_INCLUDE_DIRS})
message(${PYTHON_LIBRARIES})
message(${PYTHON_EXECUTABLE})

ecbuild_add_library( TARGET   ecflow
                     TYPE     MODULE
                     SOURCES  ${srcs}
                     LIBS     core nodeattr node libparser base libsimu libclient 
                              ${Boost_SYSTEM_LIBRARY}
                              ${Boost_SERIALIZATION_LIBRARY}
                              ${Boost_FILESYSTEM_LIBRARY}
                              ${Boost_PROGRAM_OPTIONS_LIBRARY}
                              ${Boost_DATE_TIME_LIBRARY}
                              ${Boost_PYTHON_LIBRARY} 
                              ${PYTHON_LIBRARIES}
                     INCLUDES ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ../Base/src/cts
                            ../Base/src/stc
                            ../Client/src
                            ../CSim/src
                            ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} )

# by default cmake add prefix 'lib', we don't want this hence disable
set_target_properties(ecflow PROPERTIES PREFIX "")

### tests 

list( APPEND tests
        py_u_TestAddDelete   
        py_u_TestAddDeleteFunc 
        py_u_TestAddNodeFunc  
        py_s_TestClientApi    
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
set(PYTHON_SITE "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages" )
set(PYTHON_DEST "${PYTHON_SITE}/ecflow" )

install( TARGETS ecflow       DESTINATION ${PYTHON_DEST} 
         RENAME ecflow.so
       )
install( FILES   ecflow/__init__.py  DESTINATION ${PYTHON_DEST} )


##############################################################################
# Install using setup.py

#if (ENABLE_PYTHON)
#    message(STATUS "python found, install python extension")
#
#    #set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
#    set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
#    set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/ecflow/__init__.py")
#    set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

#    # configure_file(${SETUP_PY_IN} ${SETUP_PY})

#    add_custom_command(OUTPUT ${OUTPUT}
#                       # COMMAND ${PYTHON} ${SETUP_PY} build
#                       COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
#                       DEPENDS ${DEPS})
#
#    add_custom_target(target ALL DEPENDS ${OUTPUT})
#
#    install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
#endif()


 
