#ifndef EVENT_NODE_H
#define EVENT_NODE_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #10 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#include "node.h"
#include "show.h"

class event_node : public node {

  virtual void info(std::ostream&){}
    virtual bool evaluate() const;

    virtual void drawNode(Widget w,XRectangle* r,bool);
    virtual void sizeNode(Widget w,XRectangle* r,bool);

    virtual Boolean visible() const { return show::want(show::event); }

    const char* status_name() const;

    virtual void perlify(FILE*);

public:
    event_node(host& h,ecf_node* n);
#ifdef BRIDGE
    event_node(host& h,sms_node* n,char b) : node(h,n,b) {}
#endif
};

#endif
