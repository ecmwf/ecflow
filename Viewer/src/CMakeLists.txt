
set(viewer_srcs   
    ViewerMain.cpp
    AboutDialog.cpp
    AbstractNodeModel.cpp
    AbstractSearchLine.cpp
    AbstractTextEditSearchInterface.cpp
    ActionHandler.cpp
    Animation.cpp
    AttributeEditor.cpp
    AttributeEditorFactory.cpp
    AttributeSearchPanel.cpp
    CaseSensitiveButton.cpp
    ChangeNotify.cpp
    ChangeNotifyDialog.cpp
    ChangeNotifyEditor.cpp
    ChangeNotifyModel.cpp
    ChangeNotifyWidget.cpp
    CodeItemWidget.cpp
    ConnectState.cpp
    ComboMulti.cpp
    CommandDesignerWidget.cpp
    ConfigListDelegate.cpp
    CustomCommandDialog.cpp
    CustomCommandHandler.cpp
    CustomListWidget.cpp
    CustomTabWidget.cpp
    Dashboard.cpp
    DashboardDialog.cpp
    DashboardDock.cpp
    DashboardTitle.cpp
    DashboardWidget.hpp
    DirectoryHandler.cpp
    EditItemWidget.cpp
    EditProvider.cpp
    EditorInfoLabel.cpp
    ExpandState.cpp
    FlagSet.hpp
    FileInfoLabel.cpp
    FileWatcher.cpp
    FilterWidget.cpp
    GotoLineDialog.cpp
    Highlighter.cpp
    HistoryItemWidget.cpp
    IconProvider.cpp
    InfoPanel.cpp 
    InfoPanelItem.cpp   
    InfoPanelHandler.cpp
    InfoProvider.cpp
    JobItemWidget.cpp
    LabelEditor.cpp
    LimitEditor.cpp
    LineEdit.cpp
    LogEvent.cpp
    LogModel.cpp
    LogProvider.cpp
    LogServer.cpp
    MainWindow.cpp
    ManualItemWidget.cpp 
    MenuConfigDialog.cpp
    MenuHandler.cpp
    MessageLabel.cpp
    MessageItemWidget.cpp
    MeterEditor.cpp
    ModelColumn.cpp
    NodeExpression.cpp
    NodeFilterDialog.cpp    
    NodePanel.cpp
    NodePathWidget.cpp
    NodeQuery.cpp
    NodeQueryCombo.cpp
    NodeQueryEditor.cpp
    NodeQueryEngine.cpp
    NodeQueryHandler.cpp
    NodeQueryResult.cpp
    NodeQueryResultModel.cpp
    NodeQueryResultView.cpp
    NodeQueryViewDelegate.cpp
    NodeSearchDialog.cpp
    NodeSearchWidget.cpp
    NodeViewBase.cpp
    NodeViewDelegate.cpp
    NodeWidget.cpp
    OneLineTextEdit.cpp
    OutputBrowser.cpp
    OutputCache.cpp
    OutputClient.cpp
    OutputFetchInfo.cpp
    OutputFileClient.cpp
    OutputDirClient.cpp
    OutputModel.cpp
    OutputItemWidget.cpp
    OutputFileProvider.cpp
    OutputDirProvider.cpp
    OverviewItemWidget.cpp
    OverviewProvider.cpp
    Palette.cpp
    PlainTextEdit.cpp
    PlainTextSearchLine.cpp
    PlainTextSearchInterface.cpp
    PropertyDialog.cpp
    PropertyEditor.cpp
    PropertyLine.cpp
    PropertyMapper.cpp
    RepeatEditor.cpp
    SaveSessionAsDialog.cpp
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
    SessionDialog.cpp
    SessionRenameDialog.cpp
    SessionHandler.cpp
    Sound.cpp
    StringMatchCombo.cpp
    StringMatchMode.cpp
    SuiteItemWidget.cpp
    SuiteFilter.cpp
    SuiteFilterObserver.hpp
    SuiteModel.cpp
    TabWidget.cpp 
    TableFilterWidget.cpp
    TableNodeModel.cpp
    TableNodeSortModel.cpp
    TableNodeView.cpp
    TableNodeViewDelegate.cpp
    TableNodeWidget.cpp
    TextEditSearchLine.cpp
    TimeItemWidget.cpp
    TriggerItemWidget.cpp
    TreeNodeModel.cpp
    TreeNodeView.cpp
    TreeNodeViewDelegate.cpp
    TreeNodeWidget.cpp
    TreeView.cpp
    TriggerView.cpp
    UpdateTimer.cpp
    UserMessage.cpp
    VAttribute.cpp
    VAttributeType.cpp
    VConfig.cpp   
    VDir.cpp
    VFilter.cpp
    VIcon.cpp
    VInfo.cpp
    VFile.cpp
    VFileInfo.cpp
    VModelData.cpp
    VNode.cpp
    VNodeList.cpp
    VNState.cpp
    VParam.cpp
    VProperty.cpp   
    VRepeat.cpp
    VReply.cpp
    VServerSettings.cpp
    VSettings.cpp
    VSState.cpp
    VTask.cpp
    VTaskNode.cpp
    VTree.cpp
    VariableModel.cpp
    VariableModelData.cpp
    VariableItemWidget.cpp
    VariableSearchLine.cpp
    VariableView.cpp
    VConfigLoader.cpp
    WhyItemWidget.cpp
    ZombieItemWidget.cpp
    ZombieModel.cpp
    TextPager/TextPagerCursor.cpp
    TextPager/TextPagerDocument.cpp
    TextPager/TextPagerEdit.cpp
    TextPager/TextPagerLayout_p.cpp
    TextPager/TextPagerSearchHighlighter.cpp
    TextPager/TextPagerSearchInterface.cpp
    TextPager/TextPagerSection.cpp
    TextPager/TextPagerWidget.cpp
    TextPager/syntaxhighlighter.cpp)

if(ECFLOW_QT5)
    include_directories(${ECFLOW_QT_INCLUDE_DIR})
else()
    include(${QT_USE_FILE}) 
    set(ECFLOW_QT_LIBRARIES ${QT_LIBRARIES})
endif()


set(viewer_moc_files AbstractNodeModel.hpp
    AbstractSearchLine.hpp
    ActionHandler.hpp
    Animation.hpp
    AttributeEditor.hpp
    AttributeSearchPanel.hpp
    CaseSensitiveButton.hpp
    ChangeNotifyDialog.hpp
    ChangeNotifyEditor.hpp
    ChangeNotifyModel.hpp
    ChangeNotifyWidget.hpp
    ComboMulti.hpp
    CommandDesignerWidget.hpp
    CodeItemWidget.hpp
    CustomCommandDialog.hpp
    CustomListWidget.hpp
    Dashboard.hpp
    DashboardDialog.hpp
    DashboardDock.hpp
    DashboardTitle.hpp
    DashboardWidget.hpp
    EditItemWidget.hpp
    FileWatcher.hpp
    FilterWidget.hpp
    GotoLineDialog.hpp
    HistoryItemWidget.hpp              
    InfoPanel.hpp
    LabelEditor.hpp
    LimitEditor.hpp
    LineEdit.hpp
    LogProvider.hpp
    MainWindow.hpp
    MenuConfigDialog.hpp
    MeterEditor.hpp
    NodeFilterDialog.hpp
    NodePanel.hpp
    NodePathWidget.hpp
    NodeQueryCombo.hpp           
    NodeQueryEditor.hpp
    NodeQueryEngine.hpp
    NodeQueryResult.hpp
    NodeQueryResultModel.hpp
    NodeQueryResultView.hpp
    NodeSearchDialog.hpp
    NodeSearchWidget.hpp
    NodeWidget.hpp
    OneLineTextEdit.hpp
    OutputBrowser.hpp
    OutputCache.hpp
    OutputClient.hpp
    OutputFileClient.hpp
    OutputDirClient.hpp
    OutputItemWidget.hpp
    OutputFileProvider.hpp
    OutputDirProvider.hpp
    PlainTextEdit.hpp
    PropertyDialog.hpp             
    PropertyEditor.hpp
    PropertyLine.hpp
    SaveSessionAsDialog.hpp
    RepeatEditor.hpp
    ServerComQueue.hpp
    ServerComThread.hpp
    ServerHandler.hpp
    ServerListDialog.hpp
    ServerSettingsItemWidget.hpp
    SessionDialog.hpp
    SessionRenameDialog.hpp
    StringMatchCombo.hpp
    SuiteItemWidget.hpp
    TabWidget.hpp
    TableFilterWidget.hpp
    TableNodeModel.hpp 
    TableNodeView.hpp 
    TableNodeViewDelegate.hpp
    TableNodeWidget.hpp             
    TextEditSearchLine.hpp
    TreeNodeModel.hpp
    TreeNodeView.hpp           
    TreeNodeViewDelegate.hpp
    TreeNodeWidget.hpp
    VariableItemWidget.hpp
    VariableModel.hpp
    VariableModelData.hpp
    VariableSearchLine.hpp
    VFilter.hpp
    VModelData.hpp
    VNodeList.hpp
    ZombieItemWidget.hpp
    TextPager/TextPagerCursor_p.hpp
    TextPager/TextPagerDocument.hpp
    TextPager/TextPagerDocument_p.hpp
    TextPager/TextPagerEdit.hpp
    TextPager/TextPagerEdit_p.hpp
    TextPager/TextPagerSection_p.hpp
    TextPager/TextPagerWidget.hpp
    TextPager/syntaxhighlighter.hpp   
)

set(viewer_wrap_ui_files
    AboutDialog.ui
    AttributeEditorDialog.ui
    ChangeNotifyDialog.ui
    ChangeNotifyDialogWidget.ui
    ChangeNotifyEditor.ui
    CommandDesignerWidget.ui 
    CustomCommandDialog.ui 
    CodeItemWidget.ui
    DashboardDialog.ui
    DashboardDockTitleWidget.ui
    EditItemWidget.ui
    GotoLineDialog.ui
    HistoryItemWidget.ui
    InfoPanel.ui    
    LabelEditorWidget.ui
    LimitEditorWidget.ui
    MainWindow.ui
    MessageItemWidget.ui
    MenuConfigDialog.ui
    MeterEditorWidget.ui
    NodeFilterDialog.ui
    NodeQueryEditor.ui
    NodeQuerySaveDialog.ui
    NodeSearchDialog.ui
    NodeSearchWidget.ui
    OutputItemWidget.ui 
    PropertyDialog.ui
    PropertyEditor.ui
    RepeatEditorWidget.ui
    SaveSessionAsDialog.ui
    SearchLineWidget.ui
    ServerAddDialog.ui
    ServerEditDialog.ui 
    ServerListDialog.ui
    ServerSettingsItemWidget.ui
    SessionDialog.ui
    SessionRenameDialog.ui
    SuiteItemWidget.ui
    TableFilterWidget.ui
    TableNodeWidget.ui
    TimeItemWidget.ui
    TreeNodeWidget.ui
    TriggerItemWidget.ui          
    VariableAddDialog.ui
    VariablePropDialog.ui
    VariableItemWidget.ui
    ZombieItemWidget.ui
)

if(ECFLOW_QT5)
  QT5_WRAP_CPP(VIEWER_MOC ${viewer_moc_files} )
  QT5_ADD_RESOURCES (VIEWER_RES viewer.qrc)
  QT5_WRAP_UI       (VIEWER_FORMS_HEADERS ${viewer_wrap_ui_files})
else()
  QT4_WRAP_CPP(VIEWER_MOC ${viewer_moc_files} )
  QT4_ADD_RESOURCES (VIEWER_RES viewer.qrc)
  QT4_WRAP_UI       (VIEWER_FORMS_HEADERS ${viewer_wrap_ui_files})
endif()


# add all the images as dependencies of the resource file so that it is
# automatically recompiled when an image changes
file( GLOB image_files "${CMAKE_CURRENT_SOURCE_DIR}/../images/*.*" )
ADD_CUSTOM_TARGET(Qt_resource_cpp DEPENDS ${VIEWER_RES})
ADD_DEPENDENCIES(Qt_resource_cpp ${image_files})



add_definitions( -DECFLOW_SHARED_DIR="${CMAKE_INSTALL_PREFIX}/share/ecflow" )


ecbuild_add_executable( TARGET   ecflow_ui.x
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
     TextPager    
     ${Boost_INCLUDE_DIRS} 
     ${CMAKE_CURRENT_BINARY_DIR}
    LIBS core nodeattr node libparser base libclient 
         pthread  
         m crypt dl fl
         ${ECFLOW_QT_LIBRARIES}
)

# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs
target_link_libraries(ecflow_ui.x debug ${Boost_REGEX_LIBRARY_DEBUG}   ${Boost_REGEX_LIBRARY_RELEASE} )


set(build_ecflow_ui_log 0)

if(${build_ecflow_ui_log})
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
        LIBS core nodeattr node libparser base libclient 
             pthread  
             m 
             ${ECFLOW_QT_LIBRARIES}
        DEFINITIONS MAIN_LOG
    )
endif()
