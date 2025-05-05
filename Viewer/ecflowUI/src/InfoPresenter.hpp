/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_InfoPresenter_HPP
#define ecflow_viewer_InfoPresenter_HPP

#include <vector>

#include "VInfo.hpp"

class InfoProvider;
class VReply;

// This is a base class for presenting a VInfo object. The InfoPanelItems
// are derived from this class. It can contain an InfoProvider member that
// is able to generate the information we want to display about the VInfo.

class InfoPresenter {
public:
    InfoPresenter() = default;
    virtual ~InfoPresenter();
    virtual void infoReady(VReply*) {}
    virtual void infoFailed(VReply*) {}
    virtual void infoProgress(VReply*) {}
    virtual void infoProgressUpdate(const std::string& /*text*/, int /*value*/) {}
    virtual void infoProgressStart(const std::string& /*text*/, int /*max*/) {}
    virtual void infoProgressStop() {}
    virtual void infoAppended(VReply*) {}
    VInfo_ptr info() const { return info_; }
    void registerInfoProvider(InfoProvider* ip) { infoProviders_.push_back(ip); }

protected:
    VInfo_ptr info_;
    InfoProvider* infoProvider_{nullptr};      // the main info provider
    std::vector<InfoProvider*> infoProviders_; // the list of all the providers including the main one
};

#endif /* ecflow_viewer_InfoPresenter_HPP */
