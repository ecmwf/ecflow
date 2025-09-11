/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "NodeQueryHandler.hpp"

#include <algorithm>

#include "DirectoryHandler.hpp"
#include "NodeQuery.hpp"
#include "ecflow/core/File.hpp"

NodeQueryHandler* NodeQueryHandler::instance_ = nullptr;

NodeQueryHandler::NodeQueryHandler() : suffix_("query") {
}

NodeQueryHandler* NodeQueryHandler::instance() {
    if (!instance_) {
        instance_ = new NodeQueryHandler();
    }

    return instance_;
}

NodeQuery* NodeQueryHandler::add(const std::string& name) {
    auto* item = new NodeQuery(name);
    items_.push_back(item);
    return item;
}

NodeQuery* NodeQueryHandler::add(const std::string& name, const std::string& /*query*/) {
    auto* item = new NodeQuery(name);
    items_.push_back(item);
    return item;
}

void NodeQueryHandler::add(NodeQuery* item, bool saveToFile) {
    items_.push_back(item);
    if (saveToFile) {
        save(item);
    }
}

void NodeQueryHandler::remove(const std::string&) {
}

void NodeQueryHandler::remove(NodeQuery*) {
}

NodeQuery* NodeQueryHandler::find(const std::string& name) const {
    for (auto item : items_) {
        if (item->name() == name) {
            return item;
        }
    }
    return nullptr;
}

void NodeQueryHandler::save() {
}

void NodeQueryHandler::save(NodeQuery* item) {
    std::string f = DirectoryHandler::concatenate(dirPath_, item->name() + "." + suffix_);
    VSettings vs(f);
    item->save(&vs);
    vs.write();
}

void NodeQueryHandler::init(const std::string& dirPath) {
    dirPath_ = dirPath;
    DirectoryHandler::createDir(dirPath_);

    std::vector<std::string> res;
    std::string pattern = ".*\\." + suffix_ + "$";
    DirectoryHandler::findFiles(dirPath_, pattern, res);

    for (auto it = res.begin(); it != res.end(); ++it) {
        std::string fName = DirectoryHandler::concatenate(dirPath_, *it);
        VSettings vs(fName);
        if (vs.read(false)) {
            std::size_t pos = (*it).find("." + suffix_);
            assert(pos != std::string::npos);
            std::string name = (*it).substr(0, pos);
            NodeQuery* item  = add(name);
            item->load(&vs);
        }
    }
}
