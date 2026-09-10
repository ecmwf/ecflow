/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeQueryResultTmp_HPP
#define ecflow_viewer_NodeQueryResultTmp_HPP

#include <memory>

#include <QString>
#include <QStringList>

class VNode;

struct NodeQueryResultTmp;
using NodeQueryResultTmp_ptr = std::shared_ptr<NodeQueryResultTmp>;

struct NodeQueryResultTmp
{
    NodeQueryResultTmp() = default;
    explicit NodeQueryResultTmp(VNode* node)
        : node_(node) {}
    NodeQueryResultTmp(VNode* node, QStringList attr)
        : node_(node),
          attr_(attr) {}

    VNode* node_{nullptr};
    QStringList attr_;
};

#endif /* ecflow_viewer_NodeQueryResultTmp_HPP */
