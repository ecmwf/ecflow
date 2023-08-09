//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef PLAINTEXTWIDGET_HPP
#define PLAINTEXTWIDGET_HPP

#include <QWidget>

namespace Ui {
class PlainTextWidget;
}

class PlainTextWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlainTextWidget(QWidget* parent = nullptr);
    ~PlainTextWidget() override = default;

    void setPlainText(QString);
    void setTitle(QString);
    void setShowTitleLabel(bool);

protected Q_SLOTS:
    void slotSearch();
    void slotGotoLine();
    void slotFontSizeUp();
    void slotFontSizeDown();

private:
    Ui::PlainTextWidget* ui_;
};

#endif // PLAINTEXTWIDGET_HPP
