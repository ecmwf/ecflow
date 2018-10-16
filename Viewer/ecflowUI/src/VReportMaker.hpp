//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VREPORTMAKER_HPP
#define VREPORTMAKER_HPP

#include <QObject>

#include "VFile.hpp"
#include "VInfo.hpp"
#include "InfoPresenter.hpp"

class OutputFileProvider;

class VReportMaker : public InfoPresenter, public QObject
{
public:
    static void sendReport(VInfo_ptr);

    //From VInfoPresenter
    void infoReady(VReply*);
    void infoFailed(VReply*);
    //void infoProgress(VReply*);
    //void infoProgressStart(const std::string& text,int max);
    //void infoProgress(const std::string& text,int value);

protected:
     VReportMaker(QObject* parent=0);
     void run(VInfo_ptr);
     void loadFile(VFile_ptr file);

     OutputFileProvider *infoProvider_;
};


#endif // VREPORTMAKER_HPP
