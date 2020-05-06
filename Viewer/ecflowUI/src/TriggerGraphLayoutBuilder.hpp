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
    int indexOfParent(int v) const {
        for(size_t i=0; i < parents_.size(); i++) {
            if (parents_[i] == v)
                return static_cast<int>(i);
        }
        return -1;
    }

    int indexOfChild(int v) const {
        for(size_t i=0; i < children_.size(); i++) {
            if (children_[i] == v)
                return static_cast<int>(i);
        }
        return -1;
    }

    int x_ {0};
    int y_ {0};
    int width_ {0};
    int height_ {0};
    std::vector<int> parents_;
    std::vector<int> children_;
};

class GraphLayoutEdge
{
public:
    GraphLayoutEdge(int from, int to) : from_(from), to_(to) {}

    int from_;
    int to_;
    std::vector<int> x_;
    std::vector<int> y_;
    std::vector<int> width_;
};

struct SimpleGraphLayoutNode : GraphLayoutNode
{
    SimpleGraphLayoutNode(GraphLayoutNode* n) : GraphLayoutNode(n) {}
    SimpleGraphLayoutNode() : GraphLayoutNode(0, 0) {}

    int arc_ {0};
    int level_ {0};
    int group_ {0};
    bool visited_ {false};
    bool managed_ {true};
    bool dummy_ {false};
};


class GraphLayoutBuilder
{
public:
    GraphLayoutBuilder() {}
    virtual ~GraphLayoutBuilder() {}
    virtual void clear() =0;
    virtual void build(std::vector<GraphLayoutNode*>&, std::vector<GraphLayoutEdge*>&, int focus) =0;
    void setXMinGap(int v) {xMinGap_ = v;}
    void setyMinGap(int v) {yMinGap_ = v;}

protected:
    int xMinGap_  {60};
    int yMinGap_  {10};
};

class SimpleGraphLayoutBuilder : public GraphLayoutBuilder
{
public:
    SimpleGraphLayoutBuilder() {}
    ~SimpleGraphLayoutBuilder() override;
    void clear() override;
    void build(std::vector<GraphLayoutNode*>& nodes, std::vector<GraphLayoutEdge*>& edges, int focus) override;

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
    void buildIt(bool dummy);

    int insertDummyNode(int parentIndex, int chIndex, int level);
    bool addDummy(int nodeIndex);
    bool addDummyNodes();
    void printState(const std::vector<int>& nodes);

    std::vector<SimpleGraphLayoutNode*> nodes_;
    int focus_ {0};
};


#endif // TRIGGERGRAPHLAYOUTBUILDER_HPP
