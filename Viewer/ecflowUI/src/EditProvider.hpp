/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_EditProvider_HPP
#define ecflow_viewer_EditProvider_HPP

#include "InfoProvider.hpp"
#include "VDir.hpp"
#include "VInfo.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class EditProvider : public InfoProvider {
public:
    explicit EditProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::OutputTask), preproc_(false) {}

    void visit(VInfoNode*) override;
    void submit(const std::vector<std::string>& txt, bool alias);

    void preproc(bool b) { preproc_ = b; }

private:
    bool preproc_;
};

#endif /* ecflow_viewer_EditProvider_HPP */
