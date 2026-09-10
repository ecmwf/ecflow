/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ActionHandler_HPP
#define ecflow_viewer_ActionHandler_HPP

#include <vector>

#include <QObject>
#include <QPoint>

#include "VInfo.hpp"

class QWidget;
class Node;
class ServerHandler;
class MenuItem;

class ActionHandler : public QObject {
    Q_OBJECT
public:
    explicit ActionHandler(QObject*, QWidget* menuParent);

    void contextMenu(const std::vector<VInfo_ptr>&, QPoint);
    void runCommand(const std::vector<VInfo_ptr>& nodesLst, int commandId);
    bool actionHandler();

    static bool confirmCommand(const std::vector<VInfo_ptr>& filteredNodes,
                               const std::string& questionIn,
                               const std::string& warning,
                               const std::string& commandDescStr,
                               std::size_t taskNum);

    void setallowShortcutsForHiddenItems(bool v) { allowShortcutsForHiddenItems_ = v; }

Q_SIGNALS:
    void viewCommand(VInfo_ptr, QString);
    void infoPanelCommand(VInfo_ptr, QString);
    void dashboardCommand(VInfo_ptr, QString);

protected:
    void filterNodes(const std::vector<VInfo_ptr>& nodesLst, std::vector<VInfo_ptr>&) const;
    void handleCommand(MenuItem* item, const std::vector<VInfo_ptr>& filteredNodes);
    bool confirmCommand(MenuItem* item,
                        const std::vector<VInfo_ptr>& filteredNodes,
                        const std::string& commandDescStr = std::string(),
                        std::size_t task_num              = 0);

    QObject* actionSender_;
    QWidget* menuParent_;
    bool allowShortcutsForHiddenItems_{false};
};

#endif /* ecflow_viewer_ActionHandler_HPP */
