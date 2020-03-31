//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERGRAPHLAYOUTBUILDER_HPP
#define TRIGGERGRAPHLAYOUTBUILDER_HPP

#include <vector>

class TriggerGraphLayoutNode;

struct GraphLayoutNode
{
    GraphLayoutNode(int width, int height) : width_(width), height_(height) {}
    GraphLayoutNode(GraphLayoutNode* n) :
        width_(n->width_), height_(n->height_),
        parents_(n->parents_), children_(n->children_) {}

    bool hasParents() const {return parents_.size() > 0;}
    bool hasChildren() const {return children_.size() > 0;}

    int x_ {0};
    int y_ {0};
    int width_ {0};
    int height_ {0};
    std::vector<int> parents_;
    std::vector<int> children_;
};


struct SimpleGraphLayoutNode : GraphLayoutNode
{
    SimpleGraphLayoutNode(GraphLayoutNode* n) : GraphLayoutNode(n) {}

    int arc_ {0};
    int level_ {0};
    int group_ {0};
    bool visited_ {false};
    bool managed_ {true};
};


class GraphLayoutBuilder
{
public:
    GraphLayoutBuilder() {}
    virtual ~GraphLayoutBuilder() {}
    virtual void clear() =0;
    virtual void build(std::vector<GraphLayoutNode*>&) =0;
};

class SimpleGraphLayoutBuilder : public GraphLayoutBuilder
{
public:
    SimpleGraphLayoutBuilder() {}
    ~SimpleGraphLayoutBuilder() override;
    void clear() override;
    void build(std::vector<GraphLayoutNode*>& nodes) override;

protected:
    bool hasManagedParent(SimpleGraphLayoutNode*) const;
    bool hasManagedChild(SimpleGraphLayoutNode*) const;
    int compute_level(SimpleGraphLayoutNode *item);
    void compute_level_pass2(SimpleGraphLayoutNode *item);
    void set_arc(SimpleGraphLayoutNode* item, int arc);
    int compute_arc(SimpleGraphLayoutNode *item);
    void compute_y(
            const std::vector<int>& nodes, const std::vector<int>& levels,
            std::vector<int>& positions, int max_in_a_level, int no_levels,
            int v_dist);
    void buildIt();

    std::vector<SimpleGraphLayoutNode*> nodes_;
};


#endif // TRIGGERGRAPHLAYOUTBUILDER_HPP
