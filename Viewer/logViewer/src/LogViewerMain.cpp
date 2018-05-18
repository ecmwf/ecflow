//============================================================================
// Copyright 2009-2018 ECMWF.
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
#include <QDebug>
#include <QFile>
#include <QStyleFactory>
#include <QPixmap>

//#include "File.hpp"
#include "LogMainWindow.hpp"
//#include "ServerHandler.hpp"
//#include "MenuHandler.hpp"
//#include "InfoPanelHandler.hpp"
//#include "InputEventLog.hpp"
#include "DirectoryHandler.hpp"
//#include "Highlighter.hpp"
//#include "NodeQueryHandler.hpp"
//#include "CustomCommandHandler.hpp"
#include "Palette.hpp"
//#include "ServerList.hpp"
//#include "VConfig.hpp"
//#include "VIcon.hpp"
//#include "VServerSettings.hpp"
//#include "SessionHandler.hpp"
//#include "SessionDialog.hpp"
#include "UiLog.hpp"

int main(int argc, char **argv)
{
    QString host, port, name, logFile;
    if (argc == 5)
    {
        name=QString(argv[1]);
        host=QString(argv[2]);
        port=QString(argv[3]);
        logFile=QString(argv[4]);
    }

    //  std::cout << "Usage:" << std::endl;
     //   std::cout << argv[0] << " <host> <port>" << std::endl;
    //    return 1;
    //

    //Init qt
    QApplication app(argc, argv);

    app.setWindowIcon(QPixmap(":/viewer/logo_small.png"));

    QStringList styleLst=QStyleFactory::keys();

    //Set the style
    QString style="Plastique";
    if(styleLst.contains(style))
    {
        app.setStyle(style);
    }
    else
    {
        style="Fusion";
        if(styleLst.contains(style))
        {
            app.setStyle(style);
        }
    }

    //Set font size for application
    //QFont font=app.font();
    //font.setPointSize(9);
    //app.setFont(font);

    //Initialise the config and other paths
    std::string exe(argv[0]);
    DirectoryHandler::init(exe);  // we need to tell the Directory class where we started from

    //Set the stylesheet
    std::string styleSheetFileName="viewer.qss";
    std::string styleSheetPath=DirectoryHandler::concatenate(DirectoryHandler::etcDir(),styleSheetFileName);

    QFile shFile(QString::fromStdString(styleSheetPath));
    if(shFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
         app.setStyleSheet(shFile.readAll());
    }
    shFile.close();

    // startup - via the session manager, or straight to the main window?
    bool startMainWindow = true;

    //Load the global configurations
    //VConfig::instance()->init(DirectoryHandler::etcDir());

    //Initialise the system palette
    Palette::load(DirectoryHandler::concatenate(DirectoryHandler::etcDir(),
              "ecflowview_palette.json"));

    //Start the GUI
    if (startMainWindow)
    {
        //Build the GUI
        LogMainWindow* win=LogMainWindow::makeWindow();
        Q_ASSERT(win);
        win->addNewTab(name,host,port,logFile);

        //LogMainWindow::init();

        //Show all the windows
        LogMainWindow::showWindows();

        //Start input event logging
        //InputEventLog::instance()->start();

        //Enable (daily) truncation for ui log
        //UiLog::enableTruncation();

        return app.exec();
    }
    else
    {
        return 0;  // user quit from within the session manager
    }
}
