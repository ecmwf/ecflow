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
    MainWindow.show();

    MainWindow.printDefTree("darth", 16755);

    return app.exec();
}
