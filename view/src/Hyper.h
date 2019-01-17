#ifndef  HYPER_H
#define  HYPER_H
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


/* 
    If you define MOTIF, the widget will inherit proprieties 
   from the XmPrimitive class : Help Callback, user data, ...
*/

#ifndef MOTIF
#define MOTIF
#endif

/*
   If your machine got regexp.h
*/


extern WidgetClass hyperWidgetClass;
typedef struct _HyperClassRec * HyperWidgetClass;
typedef struct _HyperRec      * HyperWidget;

/*
 * Define resource strings for the Hyper widget.
 */

#define XtNhighlightFont     "highlightFont"
#define XtNnormalFont        "normalFont"
#define XtNhighlightColor    "highlightColor"
#define XtNselectColor       "selectColor"
#define XtNnormalColor       "normalColor"
#define XtNactivateCallback  "activateCallback"
#define XtNzoomEffect        "zoomEffect"
#define XtCZoom              "Zoom"
#define XtNstartHighlight    "startHighlight"
#define XtNendHighlight      "endHighlight"
#define XtCTagChar           "TagChar"
#define XtNzoomSpeed         "zoomSpeed"
#define XtCZoomSpeed         "ZoomSpeed"
#ifndef XtCMargin
#define XtCMargin            "Margin"
#endif
#define XtNmargin            "margin"

/*
  Callback structure
*/

#define HYPER_REASON 1

typedef struct {
    int     reason;   /* always = HYPER_REASON                            */
    XEvent *event;    /* event                                            */
    char     *text;     /* pointer on highlighted text selected (read only) */
    int  length;    /* length of selected text                          */
}  hyperCallbackStruct;

#ifdef _NO_PROTO

extern Widget CreateHyper();
extern void HyperLoadFile();
extern void HyperSetText();
extern void HyperSetTags();
extern Boolean HyperFind();
extern char    *HyperGetText();

#else

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

    extern Widget CreateHyper(Widget parent,
        char *name,
        ArgList al,
        int ac);

    extern void HyperLoadFile(Widget widget,
        char *fname);

    extern void HyperSetText(Widget widget,
        char *text);

    extern void HyperSetTags (Widget widget,
        int start_highlight,
        int end_highlight);

    Boolean HyperGrep(Widget  widget,
        char    *word,
        Boolean ignore_case,
        Boolean from_start,
        Boolean wrap);

        char *HyperGetText(Widget widget,Boolean include_tags);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _NO_PROTO */

#define XtIsHyper(w)     XtIsSubclass(w,hyperWidgetClass)

#endif /* HYPER_H */
