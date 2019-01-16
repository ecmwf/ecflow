//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ACTIONHANDLER_HPP_
#define ACTIONHANDLER_HPP_

#include <QPoint>
#include <QObject>

#include <vector>

#include "VInfo.hpp"

class QWidget;
class Node;
class ServerHandler;

class ActionHandler : public QObject
{
Q_OBJECT
public:
        explicit ActionHandler(QObject*,QWidget* menuParent);

		void contextMenu(std::vector<VInfo_ptr>,QPoint);
        bool actionHandler();

Q_SIGNALS:
	    void viewCommand(VInfo_ptr,QString);
	    void infoPanelCommand(VInfo_ptr,QString);
	    void dashboardCommand(VInfo_ptr,QString);

protected:      
        QObject *actionSender_;
        QWidget *menuParent_;
};

#endif
