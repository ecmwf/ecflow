list( APPEND srcs
   src/Analyser.cpp
   src/AstAnalyserVisitor.cpp
   src/DefsAnalyserVisitor.cpp
   src/FlatAnalyserVisitor.cpp
   src/Simulator.cpp
   src/SimulatorVisitor.cpp
)
ecbuild_add_library( TARGET libsimu
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     libparser node nodeattr core    
                     INCLUDES src
                              ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                              ../AParser/src
                              ../Base/src
                    )


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
ecbuild_add_test( TARGET   c_csim
                  BOOST
                  SOURCES  ${test_srcs}
                  LIBS     libsimu        
                           pthread 
                  INCLUDES ../ANode/test
                 )

ecbuild_add_test( TARGET    c_csim_single
                  BOOST
                  SOURCES   test/TestSingleSimulator.cpp test/TestUtil.cpp
                  LIBS      libsimu   
                            pthread 
                  INCLUDES  ../ANode/test
                   )

set_property(TEST c_csim APPEND PROPERTY DEPENDS u_base)
set_property(TEST c_csim_single APPEND PROPERTY DEPENDS u_base)
                   