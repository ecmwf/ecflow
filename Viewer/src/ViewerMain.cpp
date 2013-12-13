//============================================================================
// Copyright 2013 ECMWF. 
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

#include "ViewerMainWindow.hpp"


int main(int argc, char **argv)
{

    if (argc != 3)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }


    QApplication app(argc, argv);
    ViewerMainWindow MainWindow;
    MainWindow.resize(800, 640);
    MainWindow.show();

    MainWindow.printDefTree(argv[1], atoi(argv[2]));

    return app.exec();
}
