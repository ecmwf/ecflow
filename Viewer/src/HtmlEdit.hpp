//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef HTMLEDIT_HPP
#define HTMLEDIT_HPP

#include <QTextBrowser>

#include "VProperty.hpp"

class HtmlEdit : public QTextBrowser, public VPropertyObserver
{
public:
    explicit HtmlEdit(QWidget* parent = 0);
    ~HtmlEdit();

    void setFontProperty(VProperty* p);
    void updateFont();
    void notifyChange(VProperty* p);

protected:
    void wheelEvent(QWheelEvent *event);

private:
    void fontSizeChangedByZoom();

    VProperty *fontProp_;
};

#endif // HTMLEDIT_HPP


