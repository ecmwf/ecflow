/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
    explicit EditProvider(InfoPresenter* owner)
        : InfoProvider(owner, VTask::OutputTask),
          preproc_(false) {}

    void visit(VInfoNode*) override;
    void submit(const std::vector<std::string>& txt, bool alias);

    void preproc(bool b) { preproc_ = b; }

private:
    bool preproc_;
};

#endif /* ecflow_viewer_EditProvider_HPP */
