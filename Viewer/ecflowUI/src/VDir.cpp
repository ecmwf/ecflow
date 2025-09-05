/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VDir.hpp"

#include <QtGlobal>
#include <boost/foreach.hpp>

#include "DirectoryHandler.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Str.hpp"

VDir::VDir(const std::string& path) : path_(path), fetchMode_(NoFetchMode) {
}

VDir::VDir(const std::string& path, const std::string& pattern)
    : path_(path),
      pattern_(pattern),
      fetchMode_(NoFetchMode) {
    reload();
}

VDir::~VDir() {
    clear();
}

void VDir::path(const std::string& path, bool doReload) {
    path_ = path;
    if (doReload) {
        reload();
    }
}

void VDir::clear() {
    for (auto& item : items_) {
        delete item;
    }
}

void VDir::addItem(const std::string& name, unsigned int size, unsigned int mtime) {
    auto* item = new VDirItem;

    fs::path p(name);
    // std::string dirName=p.parent_path().string();
    std::string fileName = p.filename().string();

    item->name_ = fileName;
    item->size_ = size;
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    item->mtime_ = QDateTime::fromSecsSinceEpoch(mtime);
#else
    item->mtime_ = QDateTime::fromTime_t(mtime);
#endif
    items_.push_back(item);
}

void VDir::reload() {
    clear();

    where_ = "localhost";

    fs::path path(path_);

    fs::directory_iterator it(path), eod;

    BOOST_FOREACH (fs::path const& p, std::make_pair(it, eod)) {
        if (is_regular_file(p) && ecf::algorithm::starts_with(p.filename().string(), pattern_)) {
            auto* item = new VDirItem;

            item->name_ = p.filename().string();
            item->size_ = fs::file_size(p);
            item->size_ = fs::file_size(p);
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
            item->mtime_ = QDateTime::fromSecsSinceEpoch(fs::last_write_time(p));
#else
            item->mtime_ = QDateTime::fromTime_t(fs::last_write_time(p));
#endif
            items_.push_back(item);
        }
    }
}

std::string VDir::fullName(int row) {
    std::string res;
    if (row >= 0 && row < static_cast<int>(items_.size())) {
        res = DirectoryHandler::concatenate(path_, items_.at(row)->name_);
    }
    return res;
}

int VDir::findByFullName(const std::string& fName) {
    for (std::size_t i = 0; i < items_.size(); i++) {
        if (fullName(i) == fName) {
            return i;
        }
    }
    return -1;
}
