/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VAttributeType_HPP
#define ecflow_viewer_VAttributeType_HPP

#include <map>
#include <string>
#include <vector>

#include "VParam.hpp"

class AttributeFilter;
class VNode;
class VAttribute;

class VAttributeType : public VParam {
public:
    ~VAttributeType() override = default;

    static std::vector<VParam*> filterItems();
    static VAttributeType* find(const std::string& name);
    static VAttributeType* find(int id);
    static const std::vector<VAttributeType*>& types() { return types_; }
    int typeId() const { return typeId_; }
    int keyToDataIndex(const std::string& key) const;
    int searchKeyToDataIndex(const std::string& key) const;
    QStringList searchKeys() const;
    virtual QString toolTip(QStringList) const { return QString(); }
    virtual QString definition(QStringList) const { return QString(); }

    static void scan(VNode* vnode, std::vector<VAttribute*>& v);
    using ScanProc = void (*)(VNode* vnode, std::vector<VAttribute*>& vec);
    ScanProc scanProc() { return scanProc_; }

    static const std::vector<std::string>& lastNames() { return lastNames_; }
    static void saveLastNames();
    static void initLastNames();

    // Called from VConfigLoader
    static void load(VProperty*);

protected:
    explicit VAttributeType(const std::string& name);

    using TypeIterator = std::vector<VAttributeType*>::const_iterator;
    std::map<std::string, int> keyToData_;
    std::map<std::string, int> searchKeyToData_;
    int dataCount_;
    int typeId_;
    ScanProc scanProc_;
    static std::vector<std::string> lastNames_;

private:
    static std::map<std::string, VAttributeType*> typesMap_;
    static std::vector<VAttributeType*> types_;
};

#endif /* ecflow_viewer_VAttributeType_HPP */
