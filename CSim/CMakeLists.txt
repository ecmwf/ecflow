list( APPEND srcs
   # HEADERS
   src/Analyser.hpp
   src/AstAnalyserVisitor.hpp
   src/DefsAnalyserVisitor.hpp
   src/FlatAnalyserVisitor.hpp
   src/Simulator.hpp
   src/SimulatorVisitor.hpp
   # SOURCES
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
                    )

target_link_libraries(libsimu node nodeattr core)
target_include_directories(libsimu PUBLIC src
                                           ../ACore/src 
                                           ../ANattr/src
                                           ../ANode/src
                                           ../Base/src
)

target_clangformat(libsimu)

list( APPEND test_srcs
   # HEADERS
   test/TestUtil.hpp
   # SOURCES
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
ecbuild_add_test( TARGET       c_csim
                  SOURCES      ${test_srcs}
                  LIBS         libsimu        
                               pthread 
                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                  INCLUDES     ../ANode/test ${Boost_INCLUDE_DIRS}
                  DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                  TEST_DEPENDS u_base
                 )

target_clangformat(c_csim CONDITION ENABLE_TESTS)

if (ENABLE_ALL_TESTS)
	ecbuild_add_test( TARGET        c_csim_single
                      SOURCES      test/TestSingleSimulator.cpp test/TestUtil.cpp
                      LIBS         libsimu   
                                   pthread 
                                   ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                      INCLUDES     ../ANode/test ${Boost_INCLUDE_DIRS}
                      DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                      TEST_DEPENDS u_base
                 )
  target_clangformat(c_csim_single CONDITION ENABLE_TESTS)
endif()
