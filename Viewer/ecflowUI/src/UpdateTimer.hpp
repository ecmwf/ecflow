//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef UPDATETIMER_HPP_
#define UPDATETIMER_HPP_

#include <QTimer>

class UpdateTimer : public QTimer
{
public:
    UpdateTimer(QObject* parent=0) : QTimer(parent) {}
    void drift(int,int);
};

#endif // UPDATETIMER_HPP_
