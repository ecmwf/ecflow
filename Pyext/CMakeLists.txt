### ecflow python bindings

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )
#find_package(PythonLibs)
message(${PYTHON_INCLUDE_DIRS})
message(${PYTHON_LIBRARIES})
message(${PYTHON_EXECUTABLE})

ecbuild_add_library( TARGET   ecflow
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

