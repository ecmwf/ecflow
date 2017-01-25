#ifndef VLATE_HPP
#define VLATE_HPP

#include "VAttribute.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Label;

class VLateAttr : public VAttribute
{
public:
    VLateAttr(VNode *parent,const std::string&);

    VAttributeType* type() const;
    QStringList data() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VLATE_HPP
