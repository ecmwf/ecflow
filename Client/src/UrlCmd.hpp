/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_UrlCmd_HPP
#define ecflow_client_UrlCmd_HPP

#include <string>

#include <boost/core/noncopyable.hpp>

#include "ecflow/node/NodeFwd.hpp"

///
/// \note Client side command only.
/// \note Placed in this category, since the server does not need to link with it.
///

class UrlCmd : private boost::noncopyable {
public:
    /// Will throw std::runtime_error if defs or node path is not correct
    UrlCmd(defs_ptr defs, const std::string& absNodePath);

    /// Will throw std::runtime_error if url cannot be formed
    std::string getUrl() const;

    /// Execute the url command
    void execute() const;

private:
    defs_ptr defs_;
    Node* node_;
};

#endif /* ecflow_client_UrlCmd_HPP */
