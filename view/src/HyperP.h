#ifndef HYPERP_H
#define HYPERP_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#ifdef MOTIF
#include <Xm/XmP.h>
#if (XmVersion >= 1002)
#include <Xm/PrimitiveP.h>
#endif
#endif

/* Hyper class : no new fileds */

typedef struct _HyperClassPart{
    int ignore;
} HyperClassPart;

typedef struct _HyperClassRec{
    CoreClassPart    core_class;
#ifdef MOTIF
    XmPrimitiveClassPart  primitive_class;
#endif
    HyperClassPart  hyper_class;
} HyperClassRec;

extern HyperClassRec hyperClassRec;

/* Text segment */

typedef struct text_segment {

    struct text_segment *next; /* Next segment */
    int          type;         /* NEWLINE, NORMAL or HIGHLIGHT */
    char         *text;        /* pointer to text */
    int          length;       /* length of text */
    int          desc;         /* font descent */
    GC           gc;           /* GC used to draw text */
    Position     x,y;          /* Position of drawn text */
    Dimension    width,height; /* Size of drawn text */
	int			 offset;       /* KELD : offset from start of widget */

} text_segment;

typedef struct _HyperPart {

    Cursor    hand;               /* Selecting cursor shape */

    Pixel     normal_color;       /* Color of the normal text */
    Pixel     highlight_color;    /* Color of the highlighted text */
    Pixel     select_color;       /* Color of the selected text */

    XFontStruct  *normal_font;    /* Font of the normal text */
    XFontStruct  *highlight_font; /* Font of the highlighted text */

    GC        normal_gc;          /* Gc for the normal text */
    GC        highlight_gc;       /* Gc for the highlighted text */

    GC        xor_gc;             /* Gc for zoom  */
    GC        select_gc;          /* Gc for select */

    Boolean  zoom;               /* zoom effect when selected */
    int      speed;              /* zoom speed                */
    char         start_of_highlight; /* start of highlighted text mark */
    char         end_of_highlight;   /* end of highlighted text mark */

    int      margin;             /* margins size */

    text_segment         *grep_seg;  /* segment where found text is */

    char                 *grep_txt;  /* pointer to found text */
    int              grep_len;   /* length of found text */
    int              grep_off;   /* offset of found text */

    Position          grep_x;    /* rectangle of founf text*/
    Position          grep_y;
    Dimension        grep_width;
    Dimension        grep_height;

    text_segment        *first_seg;     /* the text segments         */
    text_segment        *last_selected; /* last selected segment     */
    text_segment        *last_cursor;   /* last under cursor segment */
    XtCallbackList activate;         /* callback list             */

} HyperPart;

typedef struct _HyperRec {
    CorePart          core;
#ifdef MOTIF
    XmPrimitivePart  primitive;
#endif
    HyperPart        hyper;
} HyperRec;

#endif /* HYPERP_H */
