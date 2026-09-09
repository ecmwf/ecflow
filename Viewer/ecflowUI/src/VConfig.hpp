/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VConfig_HPP
#define ecflow_viewer_VConfig_HPP

#include <vector>

#include "ecflow/core/PTree.hpp"

class VProperty;
class VServerSettings;
class VSettings;

// This singleton class stores the configuration of the viewer.

class VConfig {
    friend class VServerSettings;

public:
    ~VConfig();

    static VConfig* instance();

    const std::string& appName() const { return appName_; }
    const std::string& appLongName() const { return appLongName_; }
    void init(const std::string& parDir);
    const std::vector<VProperty*>& groups() const { return groups_; }
    VProperty* find(const std::string& path);
    VProperty* cloneServerGui(VProperty* linkTarget);
    bool proxychainsUsed() const { return proxychainsUsed_; }

    void saveSettings();
    void importSettings();

protected:
    VConfig();

    void loadInit(const std::string& parFile);
    void loadProperty(const ecf::PTree& pt, VProperty* prop);
    void loadSettings();
    void saveSettings(const std::string& parFile, VProperty* guiProp, VSettings* vs, bool);
    void loadSettings(const std::string& parFile, VProperty* guiProp, bool);
    void loadImportedSettings(const ecf::PTree& pt, VProperty* guiProp);
    bool readRcFile(const std::string& rcFile, ecf::PTree& pt);

    VProperty* group(const std::string& name);

    static VConfig* instance_;

    std::string appName_{"ecFlowUI"};
    std::string appLongName_;
    std::vector<VProperty*> groups_;
    bool proxychainsUsed_{false};
};

#endif /* ecflow_viewer_VConfig_HPP */
