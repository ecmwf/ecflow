### ecflow python bindings

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )
#find_package(PythonLibs)
message(${PYTHON_INCLUDE_DIRS})
message(${PYTHON_LIBRARIES})
message(${PYTHON_EXECUTABLE})

ecbuild_add_library( TARGET   ecflow
                     NOINSTALL
                     TYPE     SHARED
                     SOURCES  ${srcs}
                     LIBS     ${Ecflow_Boost_LIBS} ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES}
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

### tests 

list( APPEND tests
        TestAddDelete   
        TestAddDeleteFunc 
        TestAddNodeFunc  
        TestClientApi    
        TestDefs         
        TestDefsCheck    
        TestDerivable     
        TestError        
        TestFind        
        TestGetAllTasks   
        TestJobGeneration 
        TestParent        
        TestRepeatArithmetic
        TestServerGetDefs
        TestSimulator  
        TestTraversal     
        TestUserManual    
        TestWith         
)

foreach( test ${tests} )

    ecbuild_add_test( TARGET ${test} 
                      TYPE PYTHON 
                      ARGS ${CMAKE_CURRENT_SOURCE_DIR}/test/${test}.py
                      ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/lib" )
endforeach()


ecbuild_add_test( TARGET TestMigrate
                  TYPE PYTHON 
                  ARGS ${CMAKE_CURRENT_SOURCE_DIR}/migrate/TestMigrate.py
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


#SET(COPY_TO_PATH ${PYTHON_DEST}/ecflow.so )
#ADD_CUSTOM_COMMAND(TARGET ecflow POST_BUILD
#                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ecflow>  ${COPY_TO_PATH}
#                   COMMENT "Copying 'ecflow' library to '${COPY_TO_PATH}'")
