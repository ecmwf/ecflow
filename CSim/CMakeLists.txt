list( APPEND srcs
   src/Analyser.cpp
   src/AstAnalyserVisitor.cpp
   src/DefsAnalyserVisitor.cpp
   src/FlatAnalyserVisitor.cpp
   src/Simulator.cpp
   src/SimulatorVisitor.cpp
)
ecbuild_add_library( TARGET   libsimu
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     node nodeattr core    
                     INCLUDES src
                              ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                              ../Base/src
                    )


list( APPEND test_srcs
   test/TestAutoArchive.cpp
   test/TestAutoCancel.cpp
   test/TestAutoRestore.cpp
   test/TestMeter.cpp
   test/TestTime.cpp
   test/TestUtil.cpp
   test/TestSimulator.cpp
   test/TestAutoCancel.cpp
   test/TestRepeat.cpp
   test/TestToday.cpp
   test/TestAnalysis.cpp
)
ecbuild_add_test( TARGET   c_csim
                  SOURCES  ${test_srcs}
                  LIBS     libsimu        
                           pthread 
                           ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                  INCLUDES ../ANode/test ${Boost_INCLUDE_DIRS}
                  TEST_DEPENDS u_base
                 )

if (ENABLE_ALL_TESTS)

	ecbuild_add_test( TARGET       c_csim_single
                      SOURCES      test/TestSingleSimulator.cpp test/TestUtil.cpp
                      LIBS         libsimu   
                                   pthread 
                                   ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                      INCLUDES     ../ANode/test ${Boost_INCLUDE_DIRS}
                      TEST_DEPENDS u_base
                 )
endif()
              