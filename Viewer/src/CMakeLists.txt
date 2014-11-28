
set(viewer_srcs   
    ViewerMain.cpp
    AbstractNodeModel.cpp
    AbstractSearchLine.cpp
    ActionHandler.cpp
    CommandDesignerWidget.cpp
    CustomCommandDialog.cpp
    DirectoryHandler.cpp
    FilterWidget.cpp
    Highlighter.cpp
    InfoItemWidget.cpp
    InfoPanel.cpp 
    InfoPanelItem.cpp
    InfoProvider.cpp
    JobItemWidget.cpp
    MainWindow.cpp
    ManualItemWidget.cpp 
    MenuConfigDialog.cpp
    MenuHandler.cpp
    MessageItemWidget.cpp
    NodeFilterModel.cpp
    NodeInfoQuery.cpp
    NodePanel.cpp
    NodePathWidget.cpp
    NodeViewBase.cpp
    NodeViewHandler.cpp
    NodeWidget.cpp
    OutputItemWidget.cpp
    PlainTextSearchLine.cpp
    ScriptItemWidget.cpp
    ServerHandler.cpp
    ServerFilter.cpp
    ServerItem.cpp
    ServerList.cpp
    ServerListDialog.cpp
    TabWidget.cpp 
    TableNodeModel.cpp
    TableNodeView.cpp
    TextItemWidget.cpp
    TriggerItemWidget.cpp
    TreeNodeModel.cpp
    TreeNodeView.cpp
    TreeNodeViewDelegate.cpp
    UserMessage.cpp
    VAttribute.cpp   
    VConfig.cpp
    VIcon.cpp
    VFilter.cpp
    VParam.cpp
    VNState.cpp
    VSState.cpp
    VariableModel.cpp
    VariableItemWidget.cpp
    ViewNodeInfo.cpp
    WhyItemWidget.cpp)


include(${QT_USE_FILE})

QT4_WRAP_CPP(VIEWER_MOC AbstractNodeModel.hpp
					    AbstractSearchLine.hpp
					    ActionHandler.hpp
                        CommandDesignerWidget.hpp
                        CustomCommandDialog.hpp
                        FilterWidget.hpp
                        InfoPanel.hpp
                        MainWindow.hpp
                        MenuConfigDialog.hpp
                        NodeFilterModel.hpp
                        NodePanel.hpp
                        NodePathWidget.hpp
                        NodeWidget.hpp
                        PlainTextSearchLine.hpp
                        ServerHandler.hpp
                        ServerListDialog.hpp
                        TabWidget.hpp
                        TableNodeView.hpp                      
                        TreeNodeView.hpp
                        TreeNodeModel.hpp
                        UserMessage.hpp
                        VariableItemWidget.hpp)

QT4_ADD_RESOURCES (VIEWER_RES viewer.qrc)
QT4_WRAP_UI       (VIEWER_FORMS_HEADERS CommandDesignerWidget.ui CustomCommandDialog.ui 
						InfoPanel.ui
						MainWindow.ui 
                        MenuConfigDialog.ui 
                        SearchLineWidget.ui 
                        ServerAddDialog.ui
                        ServerEditDialog.ui 
                        ServerListDialog.ui
                        ServerWidget.ui 
                        TextItemWidget.ui)


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
