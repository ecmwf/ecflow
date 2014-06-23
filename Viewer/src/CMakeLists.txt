
set(viewer_srcs   
    ViewerMain.cpp
    AbstractNodeModel.cpp
    ActionHandler.cpp
    DirectoryHandler.cpp
    FilterWidget.cpp
    InfoItemWidget.cpp
    InfoPanel.cpp 
    InfoPanelItem.cpp
    JobItemWidget.cpp
    MainWindow.cpp
    ManualItemWidget.cpp 
    MenuHandler.cpp
    MessageItemWidget.cpp
    NodePanel.cpp
    NodePathWidget.cpp
    NodeViewBase.cpp
    NodeViewHandler.cpp
    NodeWidget.cpp
    ScriptItemWidget.cpp
    ServerHandler.cpp
    ServerFilter.cpp
    ServerItem.cpp
    ServerList.cpp
    ServerDialog.cpp
    TabWidget.cpp 
    TableNodeModel.cpp
    TableNodeView.cpp
    TreeNodeModel.cpp
    TreeNodeView.cpp
    VariableModel.cpp
    VariableItemWidget.cpp
    ViewConfig.cpp
    ViewFilter.cpp
    ViewNodeInfo.cpp
    WhyItemWidget.cpp)


include(${QT_USE_FILE})

QT4_WRAP_CPP(VIEWER_MOC AbstractNodeModel.hpp
					    ActionHandler.hpp
                        FilterWidget.hpp
                        InfoPanel.hpp
                        MainWindow.hpp
                        NodePanel.hpp
                        NodePathWidget.hpp
                        NodeWidget.hpp
                        ServerHandler.hpp
                        ServerDialog.hpp
                        TabWidget.hpp
                        TableNodeView.hpp                      
                        TreeNodeView.hpp
                        VariableItemWidget.hpp)

QT4_ADD_RESOURCES (VIEWER_RES viewer.qrc)
QT4_WRAP_UI       (VIEWER_FORMS_HEADERS MainWindow.ui ServerDialog.ui ServerItemDialog.ui)


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
