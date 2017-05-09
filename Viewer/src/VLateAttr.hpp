#ifndef VLATE_HPP
#define VLATE_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include "LateAttr.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Label;

class VLateAttrType : public VAttributeType
{
public:
    explicit VLateAttrType();
    QString toolTip(QStringList d) const;
    QString definition(QStringList d) const;
    void encode(ecf::LateAttr* late,QStringList& data) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
};

class VLateAttr : public VAttribute
{
public:
    VLateAttr(VNode *parent,const std::string&);

    VAttributeType* type() const;
    QStringList data() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VLATE_HPP
