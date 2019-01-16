//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef RICHTEXTSEARCHLINE_HPP
#define RICHTEXTSEARCHLINE_HPP

#include <QTextBrowser>

#include "TextEditSearchLine.hpp"

class AbstractTextSearchInterface;

class RichTextSearchLine : public TextEditSearchLine
{
public:
    explicit RichTextSearchLine(QWidget *parent=0);
    ~RichTextSearchLine();
    void setEditor(QTextBrowser*);

private:
    //The interface is set internally
    void setSearchInterface(AbstractTextSearchInterface*) {}

};

#endif // RICHTEXTSEARCHLINE_HPP
