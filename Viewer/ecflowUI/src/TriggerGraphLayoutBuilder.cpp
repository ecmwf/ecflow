//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerGraphLayoutBuilder.hpp"

#include <algorithm>

#include "TriggerGraphView.hpp"
#include "TriggerItemWidget.hpp"
#include "TriggerGraphModel.hpp"
#include "TriggerGraphDelegate.hpp"
#include "VSettings.hpp"


//========================================================
//
// SimpleGraphLayoutBuilder
//
//========================================================

SimpleGraphLayoutBuilder::~SimpleGraphLayoutBuilder() {
    clear();
}

void SimpleGraphLayoutBuilder::clear()
{
    for(auto n: nodes_) {
        delete n;
    }
    nodes_.clear();
}

void SimpleGraphLayoutBuilder::build(std::vector<GraphLayoutNode*>& nodes)
{
    clear();
    for(auto n: nodes) {
        nodes_.emplace_back(new SimpleGraphLayoutNode(n));
    }

    buildIt();
    for(size_t i=0; i < nodes_.size(); i++) {
        nodes[i]->x_ = nodes_[i]->x_;
        nodes[i]->y_ = nodes_[i]->y_;
    }

    clear();
}

bool SimpleGraphLayoutBuilder::hasManagedParent(SimpleGraphLayoutNode* n) const
{
    for (auto i: n->parents_) {
        auto p = nodes_[i];
        if (p->managed_)
            return true;
    }
    return false;
}

bool SimpleGraphLayoutBuilder::hasManagedChild(SimpleGraphLayoutNode* n)  const
{
    for (auto i: n->children_) {
        auto p = nodes_[i];
        if (p->managed_)
            return true;
    }
    return false;
}

int SimpleGraphLayoutBuilder::compute_level(SimpleGraphLayoutNode *item)
{
    if (item->visited_)
        return -1;

    item->visited_ = true;

    int lvl = 0;
    for (auto i: item->parents_) {
        auto p = nodes_[i];
        if (p->managed_) {
            int lev = compute_level(p) + 1;
            lvl = std::max(lvl, lev);
        }
    }

    item->level_ = lvl;
    item->visited_ = false;

    return lvl;
}

void SimpleGraphLayoutBuilder::compute_level_pass2(SimpleGraphLayoutNode *item)
{
    int lvl1 = 12000;
    for(auto i: item->children_) {
        auto ch = nodes_[i];
        if(ch->managed_)
            lvl1 = std::min(lvl1, ch->level_);
    }
    item->level_ = lvl1 - 1;
}

void SimpleGraphLayoutBuilder::compute_y(
        const std::vector<int>& nodes, const std::vector<int>& levels,
        std::vector<int>& positions, int max_in_a_level, int levelNum,
        int v_dist)
{
    int max = max_in_a_level * v_dist;

    for(int i=0; i< levelNum; i++)
        positions[i] = ((max/levels[i]) - (max/max_in_a_level))/2;

    for(size_t i=0; i< nodes.size(); i++) {
        auto item = nodes_[nodes[i]];
        item->y_ = positions[item->level_];
        positions[item->level_] += (max / levels[item->level_]);
    }
}

void SimpleGraphLayoutBuilder::set_arc(SimpleGraphLayoutNode* item, int arc)
{
    if(item->visited_)
        return;

    item->visited_ = true;
    item->arc_ = arc;

    for(auto i: item->parents_) {
        auto p = nodes_[i];
        if(p->managed_)
            set_arc(p, arc);
    }

    item->visited_ = false;
}

int SimpleGraphLayoutBuilder::compute_arc(SimpleGraphLayoutNode *item)
{
    int a = item->arc_;

    if(item->visited_)
        return 0;

    item->visited_ = true;

    if(!item->parents_.empty()) {
        for(auto i: item->parents_) {
            auto p = nodes_[i];
            if (p->managed_) {
                int b = compute_arc(p);
                a = std::max(a, b);
            }
        }
        set_arc(item, a);
    }

    item->visited_ = false;
    return a;
}

void SimpleGraphLayoutBuilder::buildIt()
{
    int H_DIST = 10;
    int V_DIST = 10;
    int H_MIN_SPACE = 80;
    int V_MIN_SPACE = 10;
    int max_in_a_level = 0;
    int max_level = 0;
    int levelNum = 0;
    int num_nodes = 0;

    SimpleGraphLayoutNode* focus = nullptr;

    std::vector<int> nodes;
    for(size_t i=0;i < nodes_.size() ;i++) {
        auto item = nodes_[i];
        if (item->managed_)
            nodes.push_back(i);

        item->x_ = item->y_ = 0;
        item->level_ = item->arc_ = -1;
        item->visited_ = false;
    }

    if(nodes.empty())
        return;

    if(!focus)
        focus = nodes_[0];

    std::vector<int> levels(nodes.size(),0);
    std::vector<int> widths(nodes.size(),0);
    std::vector<int> heights(nodes.size(),0);
    std::vector<int> children(nodes.size(),0);
    std::vector<int> positions(nodes.size(),0);

    for(auto item: nodes_) {
        item->level_ = 0;
    }

    int arc = 1;
    for(auto i: nodes) {
        auto item = nodes_[i];
        compute_level(item);
        item->arc_ = arc++;
        H_DIST = std::max(H_DIST,item->width_);
        V_DIST = std::max(V_DIST,item->height_);
    }

    H_DIST += H_MIN_SPACE;
    V_DIST += V_MIN_SPACE;

    for(auto i: nodes) {
        auto item = nodes_[i];
        if (!hasManagedParent(item) && hasManagedChild(item))
            compute_level_pass2(item);
    }

    // sort by level
    std::sort(nodes.begin(), nodes.end(), [this](int idx1, int idx2)
    {
        return nodes_[idx1]->level_ < nodes_[idx2]->level_;
    });

    //qsort(nodes, num_nodes, sizeof(int), by_level);

    for(auto i:nodes) {
        auto item = nodes_[i];
        levels[item->level_]++;

        if(levels[item->level_] > max_in_a_level) {
            max_in_a_level = levels[item->level_];
            max_level = item->level_;
        }

        levelNum = std::max(levelNum, item->level_);

        widths[item->level_] = std::max(widths[item->level_],item->width_);
        heights[item->level_] = std::max(heights[item->level_], item->height_);
        children[item->level_] = std::max(children[item->level_], static_cast<int>(item->children_.size()));
    }

    levelNum++;

    //int a = 0;
    for (int i = 0, a = 0 ;i < levelNum;i ++) {
        int b = widths[i] + H_MIN_SPACE;

        b += children[i] * 2 ; /* +5 pixels per node with max kids in level */
        widths[i] = a;
        a += b;
        heights[i] += V_MIN_SPACE;
    }

    for (auto i: nodes) {
        auto item = nodes_[i];
        item->x_ = widths[item->level_];
    }

    for (size_t a=0; a<2; a++) {
        for (auto i: nodes) {
            auto item = nodes_[i];
            if (!hasManagedChild(item))
                compute_arc(item);
        }
        for (auto i: nodes) {
            auto item = nodes_[i];
            if (!hasManagedChild(item))
                set_arc(item, item->arc_);
        }
    }

    // sort by arc
    std::sort(nodes.begin(), nodes.end(), [this](int idx1, int idx2)
    {
        if(nodes_[idx1]->level_ != nodes_[idx2]->level_)
            return nodes_[idx1]->level_ > nodes_[idx2]->level_;

        return nodes_[idx1]->arc_ < nodes_[idx2]->arc_;
    });

    for (int a=0; a < levelNum; a++)
        if (levels[a] == 0)
            levels[a] = 1;

    compute_y(nodes, levels, positions, max_in_a_level, levelNum, V_DIST);

    //sort by tmpY
    std::sort(nodes.begin(), nodes.end(), [this](int idx1, int idx2)
    {
        if(nodes_[idx1]->level_ != nodes_[idx2]->level_)
            return nodes_[idx1]->level_ > nodes_[idx2]->level_;

        return nodes_[idx1]->y_ < nodes_[idx2]->y_;
    });

    int move_it = 0;
    int more = 1;
    int count = 0;
    while (more--) {
        count = num_nodes;
        while (count--) {
             for (auto i: nodes) {
                int n = 0;
                int y = 0;
                auto item = nodes_[i];
                if ((max_level != item->level_) ^ move_it) {
                    for (auto j: item->parents_) {
                        auto p = nodes_[j];
                        if (p->managed_ && p != focus) {
                            y += p->y_;
                            n++;
                        }
                    }

                    for (auto j: item->children_) {
                        auto p = nodes_[j];
                        if(p->managed_ && p != focus) {
                            y += p->y_;
                            n++;
                        }
                    }
                    item->y_ = n?y/n:y;
                }
            }

            //sort by tmpY
            std::sort(nodes.begin(), nodes.end(), [this](int idx1, int idx2)
            {
                 if(nodes_[idx1]->level_ != nodes_[idx2]->level_)
                     return nodes_[idx1]->level_ > nodes_[idx2]->level_;

                 return nodes_[idx1]->y_ < nodes_[idx2]->y_;
            });

            move_it = !move_it;

            bool chg = true;
            int a = 10000;
            while (chg && a--) {
                chg = false;
                for (size_t i=1; i < nodes.size() ;i++) {
                    auto item = nodes_[nodes[i]];
                    auto itemPrev = nodes_[nodes[i-1]];
                    if(itemPrev->level_ == item->level_)
                        if((item->y_ - itemPrev->y_) < heights[item->level_]) {
                            if(itemPrev != focus)
                                itemPrev->y_ -= heights[item->level_]/2;
                            if(item != focus)
                                item->y_ += heights[item->level_]/2;
                            chg = true;
                        }
                }
            }
        }
    }


    int minY = nodes_[0]->y_;
    int minX = nodes_[0]->x_;

    for (size_t i=1; i < nodes.size() ;i++) {
        auto item = nodes_[nodes[i]];
        if (item->x_ < minX)
            minX = item->x_;
        if (item->y_ < minY)
            minY = item->y_;
    }

    minX -= 20;
    minY -= 20;

    for (auto i: nodes) {
        auto item = nodes_[i];
        item->x_ -= minX;
        item->y_ -= minY;
    }

}
