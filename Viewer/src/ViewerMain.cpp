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

#include "File.hpp"
#include "Version.hpp"
#include "MainWindow.hpp"
#include "ServerHandler.hpp"
#include "MenuHandler.hpp"
#include "DirectoryHandler.hpp"
#include "ServerList.hpp"
#include "VAttribute.hpp"
#include "VIcon.hpp"
#include "VNState.hpp"
#include "VSState.hpp"

int main(int argc, char **argv)
{
	std::cout << ecf::Version::description()  << "\n";  // print the version information


    //if (argc != 3)
    //{
    //  std::cout << "Usage:" << std::endl;
     //   std::cout << argv[0] << " <host> <port>" << std::endl;
    //    return 1;
    //}

    QApplication app(argc, argv);

    //ServerHandler::addServer(argv[1],argv[2]);

    //Initialise the config and other paths
    DirectoryHandler::init(std::string(argv[0]));  // we need to tell the Directory class where we started from

    //Load the configurable menu items
    std::string menuFilename("ecflowview_menus.json");
    std::string menuPath = DirectoryHandler::concatenate(DirectoryHandler::etcDir(), menuFilename);
    MenuHandler::readMenuConfigFile(menuPath);

    //Initialise the server list
    ServerList::instance()->init();

    //Initialise the node/server state description
    VNState::init(DirectoryHandler::concatenate(DirectoryHandler::etcDir(),
    											"ecflowview_nstate.json"));

    //Initialise the node/server state description
    VSState::init(DirectoryHandler::concatenate(DirectoryHandler::etcDir(),
			      "ecflowview_sstate.json"));

    //Initialise the node attributes description
    VAttribute::init(DirectoryHandler::concatenate(DirectoryHandler::etcDir(),
		      "ecflowview_attribute.json"));

    //Initialise the node icon description
    VIcon::init(DirectoryHandler::concatenate(DirectoryHandler::etcDir(),
		      "ecflowview_icon.json"));

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


