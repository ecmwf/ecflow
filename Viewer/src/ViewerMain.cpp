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

int main(int argc, char **argv)
{

	std::cout << ecf::Version::description()  << "\n";  // print the version information


    if (argc != 3)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }

    QApplication app(argc, argv);

    ServerHandler::addServer(argv[1],argv[2]);

    // load the configurable menu items
    DirectoryHandler::setExePath(std::string(argv[0]));  // we need to tell the Directory class where we started from
    std::string menuFilename("ecflowview_menus.json");
    std::string menuPath = DirectoryHandler::concatenate(DirectoryHandler::etcDir(), menuFilename);
    MenuHandler::readMenuConfigFile(menuPath);

    MainWindow::init();

    //add splash screen here

   // MainWindow MainWindow;
    //MainWindow.resize(800, 640);
    //MainWindow.show();

    //MainWindow.printDefTree(argv[1], atoi(argv[2]));



    MainWindow::showWindows();

    return app.exec();
}


