
set(viewer_srcs
    src/ViewerMain.cpp)


ecbuild_add_executable( TARGET   ecflowviewer
                        SOURCES  ${viewer_srcs}
                        LIBS     core nodeattr node libparser base libclient 
                                 libecflowview pthread ${Ecflow_Boost_LIBS}
                                 m crypt dl Xpm Xm Xp Xext Xt X11 fl
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
                     )
