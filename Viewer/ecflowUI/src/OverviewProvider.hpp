/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_OverviewProvider_HPP
#define ecflow_viewer_OverviewProvider_HPP

#include "InfoProvider.hpp"

class InfoPanelItem;

class OverviewProvider : public InfoProvider {
public:
    explicit OverviewProvider(InfoPresenter* owner);

    // From VInfoVisitor
    void visit(VInfoServer*) override;
    void visit(VInfoNode*) override;
    void visit(VInfoAttribute*) override;

    // From VTaskObserver
    void taskChanged(VTask_ptr) override;

protected:
    void serverInfo(VInfoServer*, std::stringstream& f);
    void nodeInfo(VInfoNode*, std::stringstream& f);
};

#endif /* ecflow_viewer_OverviewProvider_HPP */
