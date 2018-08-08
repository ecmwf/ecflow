//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef EDITORINFOLABEL_HPP
#define EDITORINFOLABEL_HPP

#include <QLabel>

class EditorInfoLabel : public QLabel
{
public:
    explicit EditorInfoLabel(QWidget* parent=nullptr);
    void setInfo(QString parent,QString type);

    static QString formatKeyLabel(QString n);
    static QString formatNodeName(QString n);
    static QString formatNodePath(QString p);
};

#endif // EDITORINFOLABEL_HPP
