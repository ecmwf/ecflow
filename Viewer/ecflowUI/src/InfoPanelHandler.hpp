/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_InfoPanelHandler_HPP
#define ecflow_viewer_InfoPanelHandler_HPP

#include <string>
#include <vector>

#include "VInfo.hpp"

class BaseNodeCondition;

class InfoPanelDef {
public:
    explicit InfoPanelDef(const std::string&);

    std::string name() const { return name_; }
    std::string label() const { return label_; }
    std::string icon() const { return icon_; }
    std::string dockIcon() const { return dockIcon_; }
    std::string show() const { return show_; }
    std::string tooltip() const { return tooltip_; }
    std::string buttonTooltip() const { return buttonTooltip_; }
    bool hidden() const { return hidden_; }
    BaseNodeCondition* visibleCondition() const { return visibleCondition_; }

    void setLabel(const std::string& s) { label_ = s; }
    void setIcon(const std::string& s) { icon_ = s; }
    void setDockIcon(const std::string& s) { dockIcon_ = s; }
    void setShow(const std::string& s) { show_ = s; }
    void setTooltip(const std::string& tooltip) { tooltip_ = tooltip; }
    void setButtonTooltip(const std::string& tooltip) { buttonTooltip_ = tooltip; }
    void setVisibleCondition(BaseNodeCondition* visibleCond) { visibleCondition_ = visibleCond; }
    void setEnabledCondition(BaseNodeCondition* enabledCond) { enabledCondition_ = enabledCond; }
    void setHidden(bool b) { hidden_ = b; }

protected:
    std::string name_;
    std::string label_;
    std::string icon_;
    std::string dockIcon_;
    std::string show_;
    std::string tooltip_;
    std::string buttonTooltip_;
    bool hidden_;

    BaseNodeCondition* visibleCondition_;
    BaseNodeCondition* enabledCondition_;
};

class InfoPanelHandler {
public:
    InfoPanelHandler();

    void init(const std::string& file);
    void visible(VInfo_ptr info, std::vector<InfoPanelDef*>& lst);
    const std::vector<InfoPanelDef*>& panels() const { return panels_; }

    static InfoPanelHandler* instance();

protected:
    static InfoPanelHandler* instance_;

    std::vector<InfoPanelDef*> panels_;
};

#endif /* ecflow_viewer_InfoPanelHandler_HPP */
