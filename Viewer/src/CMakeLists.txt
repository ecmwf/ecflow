
set(viewer_srcs   
    ViewerMain.cpp
    AbstractNodeModel.cpp
    AbstractSearchLine.cpp
    ActionHandler.cpp
    Animation.cpp
    CodeItemWidget.cpp
    ConnectState.cpp
    CommandDesignerWidget.cpp
    CustomCommandDialog.cpp
    Dashboard.cpp
    DirectoryHandler.cpp
    EditItemWidget.cpp
    EditProvider.cpp
    ExpandState.cpp
    FileInfoLabel.cpp
    FilterWidget.cpp
    Highlighter.cpp
    HistoryItemWidget.cpp
    InfoPanel.cpp 
    InfoPanelItem.cpp   
    InfoPanelHandler.cpp
    InfoProvider.cpp
    JobItemWidget.cpp
    LineEdit.cpp
    LogEvent.cpp
    LogServer.cpp
    MainWindow.cpp
    ManualItemWidget.cpp 
    MenuConfigDialog.cpp
    MenuHandler.cpp
    MessageItemWidget.cpp
    NodeExpression.cpp
    NodeFilterModel.cpp
    NodePanel.cpp
    NodePathWidget.cpp
    NodeViewBase.cpp
    NodeWidget.cpp
    OutputModel.cpp
    OutputItemWidget.cpp
    OutputProvider.cpp
    OverviewItemWidget.cpp
    OverviewProvider.cpp
    PlainTextSearchLine.cpp
    PropertyDialog.cpp
    PropertyEditor.cpp
    PropertyLine.cpp
    PropertyMapper.cpp
    ScriptItemWidget.cpp
    ServerComQueue.cpp
    ServerComThread.cpp
    ServerDefsAccess.cpp
    ServerHandler.cpp
    ServerFilter.cpp
    ServerItem.cpp
    ServerList.cpp
    ServerListDialog.cpp
    ServerSettingsItemWidget.cpp
    SessionHandler.cpp
    SuiteItemWidget.cpp
    SuiteFilter.cpp
    SuiteModel.cpp
    TabWidget.cpp 
    TableNodeModel.cpp
    TableNodeView.cpp
    TableNodeWidget.cpp
    TextEdit.cpp
    TimeItemWidget.cpp
    TriggerItemWidget.cpp
    TreeNodeModel.cpp
    TreeNodeView.cpp
    TreeNodeViewDelegate.cpp
    TreeNodeWidget.cpp
    TriggerView.cpp
    UserMessage.cpp
    VAttribute.cpp
    VConfig.cpp   
    VDir.cpp
    VFilter.cpp
    VIcon.cpp
    VInfo.cpp
    VFile.cpp
    VFileInfo.cpp
    VModelData.cpp
    VNode.cpp
    VNState.cpp
    VParam.cpp
    VProperty.cpp   
    VReply.cpp
    VServerSettings.cpp
    VSettings.cpp
    VSState.cpp
    VTask.cpp
    VariableModel.cpp
    VariableModelData.cpp
    VariableItemWidget.cpp
    VariableSearchLine.cpp
    VariableView.cpp
    VConfigLoader.cpp
    WhyItemWidget.cpp
    ZombieItemWidget.cpp
    ZombieModel.cpp)

include(${QT_USE_FILE})

set(viewer_moc_files AbstractNodeModel.hpp
    AbstractSearchLine.hpp
    ActionHandler.hpp
    Animation.hpp
    CommandDesignerWidget.hpp
    CodeItemWidget.hpp
    CustomCommandDialog.hpp
    Dashboard.hpp
    DashboardWidget.hpp
    EditItemWidget.hpp
    FilterWidget.hpp              
    InfoPanel.hpp
    LineEdit.hpp
    MainWindow.hpp
    MenuConfigDialog.hpp
    NodeFilterModel.hpp            
    NodePanel.hpp
    NodePathWidget.hpp            
    NodeWidget.hpp
    OutputItemWidget.hpp
    PropertyDialog.hpp             
    PropertyLine.hpp
    PlainTextSearchLine.hpp
    ServerComQueue.hpp
    ServerComThread.hpp
    ServerHandler.hpp
    ServerListDialog.hpp
    ServerSettingsItemWidget.hpp
    SuiteItemWidget.hpp
    TabWidget.hpp
    TableNodeModel.hpp 
    TableNodeView.hpp 
    TableNodeWidget.hpp             
    TextEdit.hpp
    TreeNodeModel.hpp
    TreeNodeView.hpp           
    TreeNodeWidget.hpp
    UserMessage.hpp
    VariableItemWidget.hpp
    VariableModel.hpp
    VariableModelData.hpp
    VariableSearchLine.hpp
    VFilter.hpp
    VModelData.hpp
    ZombieItemWidget.hpp
)

set(viewer_wrap_ui_files
    CommandDesignerWidget.ui 
    CustomCommandDialog.ui 
    CodeItemWidget.ui
    EditItemWidget.ui
    InfoPanel.ui    
    MainWindow.ui 
    TimeItemWidget.ui
    MenuConfigDialog.ui
    OutputItemWidget.ui 
    PropertyDialog.ui
    PropertyEditor.ui
    SearchLineWidget.ui 
    ServerAddDialog.ui
    ServerEditDialog.ui 
    ServerListDialog.ui
    ServerSettingsItemWidget.ui
    SuiteItemWidget.ui
    TableNodeWidget.ui
    TreeNodeWidget.ui
    TriggerItemWidget.ui          
    VariableAddDialog.ui
    VariablePropDialog.ui
    VariableItemWidget.ui
    ZombieItemWidget.ui
)

if(METVIEW_QT5)
  QT5_WRAP_CPP(VIEWER_MOC ${viewer_moc_files} )
  QT5_ADD_RESOURCES (VIEWER_RES viewer.qrc)
  QT5_WRAP_UI       (VIEWER_FORMS_HEADERS ${viewer_wrap_ui_files})
else()
  QT4_WRAP_CPP(VIEWER_MOC ${viewer_moc_files} )
  QT4_ADD_RESOURCES (VIEWER_RES viewer.qrc)
  QT4_WRAP_UI       (VIEWER_FORMS_HEADERS ${viewer_wrap_ui_files})
endif()


ecbuild_add_executable( TARGET   ecflow_ui
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

list(REMOVE_ITEM viewer_srcs ViewerMain.cpp)
ecbuild_add_executable( TARGET   ecflow_ui_log
    SOURCES  LogEvent.cpp
    ${viewer_srcs} ${VIEWER_MOC} ${VIEWER_RES} ${VIEWER_FORMS_HEADERS}
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
     m 
     ${QT_LIBRARIES}
    DEFINITIONS MAIN_LOG
)
