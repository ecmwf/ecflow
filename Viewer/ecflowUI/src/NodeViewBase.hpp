/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeViewBase_HPP
#define ecflow_viewer_NodeViewBase_HPP

#include "VInfo.hpp"
#include "Viewer.hpp"

class QWidget;
class QObject;

class TableNodeSortModel;
class NodeFilterDef;
class VSettings;
class QModelIndex;

class NodeViewBase {
public:
    explicit NodeViewBase(NodeFilterDef*);
    virtual ~NodeViewBase() = default;

    virtual void reload()                         = 0;
    virtual void rerender()                       = 0;
    virtual QWidget* realWidget()                 = 0;
    virtual QObject* realObject()                 = 0;
    virtual VInfo_ptr currentSelection()          = 0;
    virtual void selectFirstServer()              = 0;
    virtual void setCurrentSelection(VInfo_ptr n) = 0;

    virtual void readSettings(VSettings* vs)  = 0;
    virtual void writeSettings(VSettings* vs) = 0;

protected:
    NodeFilterDef* filterDef_;
};

#endif /* ecflow_viewer_NodeViewBase_HPP */
