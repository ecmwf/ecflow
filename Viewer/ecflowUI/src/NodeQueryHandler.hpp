/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeQueryHandler_HPP
#define ecflow_viewer_NodeQueryHandler_HPP

#include <string>
#include <vector>

class NodeQuery;

class NodeQueryHandler {
public:
    NodeQueryHandler();

    NodeQuery* add(const std::string& name);
    NodeQuery* add(const std::string& name, const std::string& query);
    void add(NodeQuery* item, bool save);
    void remove(const std::string& name);
    void remove(NodeQuery*);
    NodeQuery* find(const std::string& name) const;

    void save();
    void save(NodeQuery*);
    void init(const std::string& dirPath);
    const std::vector<NodeQuery*>& items() const { return items_; }

    static NodeQueryHandler* instance();

protected:
    static NodeQueryHandler* instance_;

    std::string dirPath_;
    const std::string suffix_;
    std::vector<NodeQuery*> items_;
};

#endif /* ecflow_viewer_NodeQueryHandler_HPP */
