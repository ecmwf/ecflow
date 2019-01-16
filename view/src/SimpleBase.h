#ifndef SIMPLEBASE_H
#define SIMPLEBASE_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2019 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/


extern WidgetClass  simplebaseWidgetClass;

typedef struct _SimpleBaseClassRec *SimpleBaseWidgetClass;
typedef struct _SimpleBaseRec      *SimpleBaseWidget;

#define XtNselected           "selected"
#define XtCSelected           "Selected"

#define XtNblinkRate           "blinkRate"
#define XtCBlinkRate           "BlinkRate"

#define XtNblinkColor           "blinkColor"
#define XtCBlinkColor           "BlinkColor"

#define XtNpsHeader           "psHeader"
#define XtCPsHeader           "PsHeader"

#define XtNgetpsCallback		"getps"
#define XtNlinkCallback        "linkCallback"


typedef struct {
	Widget widget;
	char *name;
	char *psproc;
} getpsCallbackStruct;

typedef struct {
    int     reason; 
    XEvent *event; 
    void*   data1;
    void*   data2;
} LinkCallbackStruct;

typedef void (*DrawProc)(Widget,XRectangle*,void*);
typedef void (*SizeProc)(Widget,XRectangle*,void*);


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


void NodesRedraw(SimpleBaseWidget w, XEvent *event, Region region);
int  NodeCreate(Widget _w, DrawProc draw, SizeProc size, void *data);
void NodeChanged(Widget _w, int node);
void *NodeFind(Widget _w, XEvent *ev);
void NodeShow(Widget _w, int node);
void NodeHideAll(Widget _w);
Boolean NodeVisibility(Widget _w, int node, Boolean vis);
void NodeNewSize(Widget _w, int node);
void NodeNewSizeAll(Widget _w);
void NodeUpdate(Widget _w);
void NodeReset(Widget _w);
void NodeReserve(Widget _w, int count);

void NodeInsert(Widget _w,int pnode,int knode,int nnode);

void NodeAddRelation(Widget _w,int pnode,int knode);
void *NodeGetRelationData(Widget _w,int pnode,int knode);
void *NodeSetRelationData(Widget _w,int pnode,int knode,void*);
GC NodeGetRelationGC(Widget _w,int pnode,int knode);
GC NodeSetRelationGC(Widget _w,int pnode,int knode,GC);

void NodeSetFocus(Widget _w, int node);
int  NodeGetFocus(Widget _w);

int NodeNewGroup(Widget _w,DrawProc draw, SizeProc size, void *data);
void NodeSetGroup(Widget _w,int node,int group);
int NodeGetGroup(Widget _w,int node);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif /* SIMPLEBASE_H */
