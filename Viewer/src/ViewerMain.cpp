//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================

#include <string>
#include <iostream>

#include <QApplication>
#include <QFile>

#include "File.hpp"
#include "MainWindow.hpp"
#include "ServerHandler.hpp"
#include "MenuHandler.hpp"
#include "InfoPanelHandler.hpp"
#include "DirectoryHandler.hpp"
#include "Highlighter.hpp"
#include "ServerList.hpp"
#include "VConfig.hpp"
#include "VServerSettings.hpp"

int main(int argc, char **argv)
{
    //if (argc != 3)
    //{
    //  std::cout << "Usage:" << std::endl;
     //   std::cout << argv[0] << " <host> <port>" << std::endl;
    //    return 1;
    //}

    QApplication app(argc, argv);

    //Initialise the config and other paths
    DirectoryHandler::init(std::string(argv[0]));  // we need to tell the Directory class where we started from

    //Set the stylesheet
    std::string styleSheetFileName="viewer.qss";
    std::string styleSheetPath=DirectoryHandler::concatenate(DirectoryHandler::etcDir(),styleSheetFileName);

    QFile shFile(QString::fromStdString(styleSheetPath));
    if(shFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
    	 app.setStyleSheet(shFile.readAll());
    }
    shFile.close();

    //Load the configurable menu items
    std::string menuFilename("ecflowview_menus.json");
    std::string menuPath = DirectoryHandler::concatenate(DirectoryHandler::etcDir(), menuFilename);
    MenuHandler::readMenuConfigFile(menuPath);

    //Load the infopanel definition
    std::string panelFile = DirectoryHandler::concatenate(DirectoryHandler::etcDir(), "ecflowview_panels.json");
    InfoPanelHandler::instance()->init(panelFile);

    //Initialise the server list
    ServerList::instance()->init();

    //Load the global configurations
    VConfig::instance()->init(DirectoryHandler::etcDir());
    
    //Import server settings from the previous viewer
    if(DirectoryHandler::isFirstStartUp())
    	VServerSettings::importRcFiles();
    
    //Initialise highlighter
    Highlighter::init(DirectoryHandler::concatenate(DirectoryHandler::etcDir(),
    		      "ecflowview_highlighter.json"));

    //Build the GUI
    MainWindow::init();

    //add splash screen here

   // MainWindow MainWindow;
    //MainWindow.resize(800, 640);
    //MainWindow.show();

    //MainWindow.printDefTree(argv[1], atoi(argv[2]));

    //Show all the windows
    MainWindow::showWindows();

    return app.exec();
}
