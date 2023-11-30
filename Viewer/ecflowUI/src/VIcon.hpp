/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_VIcon_HPP
#define ecflow_viewer_VIcon_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include <QPair>
#include <QPixmap>
#include <QVariant>

#include "VParam.hpp"

class VNode;
class VParamSet;

class VIcon : public VParam {
public:
    explicit VIcon(const std::string& name);
    ~VIcon() override;

    static std::vector<VParam*> filterItems();
    static QVariantList pixmapList(VNode* vnode, VParamSet* filter);
    static int pixmapNum(VNode* vnode, VParamSet* filter);
    static QString toolTip(VNode* vnode, VParamSet* filter);
    static VIcon* find(const std::string& name);
    static void names(std::vector<std::string>&);
    static const std::vector<std::string>& lastNames() { return lastNames_; }
    static void saveLastNames();
    static void initLastNames();

    QPixmap pixmap(int size);

    // Called from VConfigLoader
    static void load(VProperty* group);

protected:
    void initPixmap();
    virtual bool show(VNode*) = 0;
    QString shortDescription() const;

    int pixId_;

    static std::map<std::string, VIcon*> items_;
    static std::vector<VIcon*> itemsVec_;
    static std::vector<std::string> lastNames_;
};

#endif /* ecflow_viewer_VIcon_HPP */
