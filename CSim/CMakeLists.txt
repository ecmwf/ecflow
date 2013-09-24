list( APPEND srcs
   src/Analyser.cpp
   src/AstAnalyserVisitor.cpp
   src/DefsAnalyserVisitor.cpp
   src/FlatAnalyserVisitor.cpp
   src/Simulator.cpp
   src/SimulatorVisitor.cpp
)
ecbuild_add_library( TARGET libsimu
                   TYPE     STATIC
                   SOURCES  ${srcs}
                   LIBS     core nodeattr node libparser ${Ecflow_Boost_LIBS}
                   INCLUDES ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ${Boost_INCLUDE_DIRS} )


list( APPEND test_srcs
   test/TestMeter.cpp
   test/TestTime.cpp
   test/TestUtil.cpp
   test/TestSimulator.cpp
   test/TestAutoCancel.cpp
   test/TestRepeat.cpp
   test/TestToday.cpp
   test/TestAnalysis.cpp
)
ecbuild_add_test( TARGET   testsimulator
                SOURCES  ${test_srcs}
                LIBS     core nodeattr node libparser base libsimu pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ${Boost_INCLUDE_DIRS} )


ecbuild_add_test( TARGET   testsingle
                SOURCES  test/TestSingleSimulator.cpp test/TestUtil.cpp
                LIBS     core nodeattr node libparser base libsimu pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ${Boost_INCLUDE_DIRS} )
