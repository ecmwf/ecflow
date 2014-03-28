
set(viewer_srcs   
    ViewerMain.cpp
    ActionHandler.cpp
    FilterWidget.cpp
    InfoPanel.cpp
    MainWindow.cpp
    NodePanel.cpp
    NodePathWidget.cpp
    NodeViewBase.cpp
    NodeViewHandler.cpp
    NodeWidget.cpp
    ServerHandler.cpp
    TabWidget.cpp 
    TableNodeModel.cpp
    TableNodeView.cpp
    TreeNodeModel.cpp
    TreeNodeView.cpp
    VariableModel.cpp
    VariableItemWidget.cpp
    ViewConfig.cpp
    ViewNodeInfo.cpp)


include(${QT_USE_FILE})

QT4_WRAP_CPP(VIEWER_MOC ActionHandler.hpp FilterWidget.hpp InfoPanel.hpp MainWindow.hpp NodePanel.hpp NodePathWidget.hpp NodeWidget.hpp TabWidget.hpp TableNodeView.hpp TreeNodeModel.hpp TreeNodeView.hpp VariableItemWidget.hpp)
QT4_ADD_RESOURCES (VIEWER_RES viewer.qrc)
QT4_WRAP_UI       (VIEWER_FORMS_HEADERS MainWindow.ui)


ecbuild_add_executable( TARGET   ecflowviewer
                        SOURCES  ${viewer_srcs} ${VIEWER_MOC} ${VIEWER_RES} ${VIEWER_FORMS_HEADERS}
                        INCLUDES .
                                 ../../ACore/src 
                                 ../../ANattr/src
                                 ../../ANode/src
                                 ../../ANode/test
                                 ../../Base/src
                                 ../../Base/src/cts
                                 ../../Base/src/stc
                                 ../../Client/src
                                 ${Boost_INCLUDE_DIRS} 
				 ${CMAKE_CURRENT_BINARY_DIR}
                        LIBS     core nodeattr node libparser base libclient 
                                 pthread ${Ecflow_Boost_LIBS}
                                 m crypt dl fl
                                 ${QT_LIBRARIES}
                     )
