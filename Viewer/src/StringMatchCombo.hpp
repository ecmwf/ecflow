//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_STRINGMATCHCOMBO_HPP_
#define VIEWER_SRC_STRINGMATCHCOMBO_HPP_

#include <QComboBox>
#include <QToolButton>

#include "StringMatchMode.hpp"

class StringMatchTb : public QToolButton
{
public:
    StringMatchTb(QWidget* parent=0);
};    

class StringMatchCombo : public QComboBox
{
Q_OBJECT

public:
	explicit StringMatchCombo(QWidget* parent=0);

	StringMatchMode::Mode matchMode(int) const;
	StringMatchMode::Mode currentMatchMode() const;
	void setMatchMode(const StringMatchMode& mode);
};

#endif /* VIEWER_SRC_STRINGMATCHCOMBO_HPP_ */
