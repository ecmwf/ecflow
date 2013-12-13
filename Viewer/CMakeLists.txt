
set(viewer_srcs
    src/ViewerMain.cpp
    src/ViewerMainWindow.cpp)


include(${QT_USE_FILE})

QT4_WRAP_CPP(VIEWER_MOC src/ViewerMainWindow.hpp)


ecbuild_add_executable( TARGET   ecflowviewer
                        SOURCES  ${viewer_srcs} ${VIEWER_MOC}
                        INCLUDES src
                                 ../ACore/src 
                                 ../ANattr/src
                                 ../ANode/src
                                 ../ANode/test
                                 ../Base/src
                                 ../Base/src/cts
                                 ../Base/src/stc
                                 ../Client/src
                                 ${Boost_INCLUDE_DIRS}
                        LIBS     core nodeattr node libparser base libclient 
                                 libecflowview pthread ${Ecflow_Boost_LIBS}
                                 m crypt dl Xpm Xm Xp Xext Xt X11 fl
                                 ${QT_LIBRARIES}
                     )
