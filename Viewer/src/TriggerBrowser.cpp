//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerBrowser.hpp"

#include "Highlighter.hpp"
#include "TriggerCollector.hpp"

TriggerBrowser::TriggerBrowser(QWidget *parent) : QWidget(parent), scanner_(0)
{
    setupUi(this);

    //Highlighter* ih=new Highlighter(triggerBrowser_->document(),"trigger");
}

void TriggerBrowser::setScanner(TriggeredScanner* scanner)
{
    scanner_=scanner;
}

void TriggerBrowser::load(VInfo_ptr info,bool dependency)
{
    VNode *n=info->node();

    std::string te,ce;
    n->triggerExpr(te,ce);

    QString s;
    if(!te.empty())
    {
        s+="<b>Trigger expression:</b><p>" + QString::fromStdString(te) + "<p>";
    }

    TriggerListCollector c(0,"",dependency);
    n->triggers(&c);

    TriggerListCollector c1(0,"",dependency);
    n->triggered(&c1,scanner_);

    triggerBrowser_->setHtml(s + c.text());
    triggeredBrowser_->setHtml(c1.text());
}
