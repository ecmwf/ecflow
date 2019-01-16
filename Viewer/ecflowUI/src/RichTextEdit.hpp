//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef RICHTEXTEDIT_HPP
#define RICHTEXTEDIT_HPP

#include <QTextBrowser>

#include "VProperty.hpp"

class RichTextEdit : public QTextBrowser, public VPropertyObserver
{
Q_OBJECT

public:
    explicit RichTextEdit(QWidget* parent = nullptr);
    ~RichTextEdit() override;

    void setFontProperty(VProperty* p);
    void updateFont();
    void notifyChange(VProperty* p) override;

public Q_SLOTS:
     void slotZoomIn();
     void slotZoomOut();

Q_SIGNALS:
    void fontSizeChangedByWheel();

private:
    void fontSizeChangedByZoom();

    VProperty *fontProp_{nullptr};
};


#endif // RICHTEXTEDIT_HPP
