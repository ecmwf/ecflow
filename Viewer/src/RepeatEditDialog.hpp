//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef REPEATEDITDIALOG_HPP
#define REPEATEDITDIALOG_HPP

#include "ui_RepeatEditDialog.h"

#include "AttributeEditor.hpp"
#include "VInfo.hpp"

class QModelIndex;
class QStringList;
class QStringListModel;
class VRepeat;

class RepeatEditDialog : public AttributeEditor, private Ui::RepeatEditDialog
{
Q_OBJECT

public:
    RepeatEditDialog(VInfo_ptr,QWidget* parent=0);
    ~RepeatEditDialog();

protected Q_SLOTS:
    void slotSelected(const QModelIndex&);
    void slotValueEdited(QString txt);

protected:
    void buildList();
    bool isListMode() const;
    void apply();

    VRepeat* repeat_;
    QStringListModel* model_;
    QStringList modelData_;
};

#endif // REPEATEDITDIALOG_HPP


