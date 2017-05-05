//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TreeNodeViewDelegate_HPP_
#define TreeNodeViewDelegate_HPP_

#include "TreeNodeViewDelegateBase.hpp"

class TreeNodeViewDelegate : public TreeNodeViewDelegateBase
{
public:
    explicit TreeNodeViewDelegate(TreeNodeModel* model,QWidget *parent=0);
	~TreeNodeViewDelegate();

    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex& index ) const;   
};

#endif



