/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #3 $                                                                    */
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

/* xec_Cursor.c */
void xec_SetWatchCursor(Widget w);
void xec_ResetCursor(Widget w);

/* xec_Label.c */
void xec_SetLabel(Widget w, const char *title);
void xec_VaSetLabel(Widget w, const char *fmt, ...);

/* xec_List.c */
void xec_RemoveListItem(Widget w, char *p);
void xec_AddListMax(Widget w, int max, char *p);
void xec_VaAddListMax(Widget w, int max, char *fmt, ...);
void xec_ListSelectAll(Widget w);
void xec_AddListItem(Widget w, char *p);
Boolean xec_AddListItemUnique(Widget w, char *p, Boolean sel);
void xec_VaAddListItem(Widget w, char *fmt, ...);
void xec_SetListItems(Widget w, XmString list[], int count);
int xec_DumpList(FILE *f, char *fmt, Widget w);
Boolean xec_ListSearch(Widget w, char *word, Boolean nocase, Boolean fromstart, Boolean wrap);
void xec_ListSelect(Widget w, int n);
void xec_ListItemSelect(Widget w, const char*);
void xec_AddFontListItem(Widget list, char *buffer, Boolean bold);
void xec_ReplaceListItem(Widget w,const char* from,const char* to);

/* xec_Regexp.c */
int xec_compile(char *w);
int xec_step(char *p);

/* xec_Strings.c */
XmString xec_NewString(const char *s);
int xec_BuildXmStringList(XmString **list, char *p, int *count);
int xec_FreeXmStringList(XmString *list, int count);
char *xec_GetString(XmString string);

/* xec_Text.c */
int regexp_find(const char *word, const char *buffer, int nocase, int *from, int *to);
char *xec_TextGetString(Widget w, long *length);
void xec_TextFreeString(char *p);
Boolean xec_TextSearch(Widget w, char *word, Boolean nocase, Boolean regex, Boolean back, Boolean fromstart, Boolean wrap);
char *xec_GetText(Widget w, char buf[]);
int xec_LoadText(Widget Text, const char *fname, Boolean include);
void *xec_MapText(Widget w, const char *fname, int *z);
void xec_UnmapText(void *x);
int xec_SaveText(Widget w, char *fname);
int xec_DumpText(FILE *fp, Widget w);
void xec_PrintText(Widget w, char *cmd);
void xec_ReplaceTextSelection(Widget w, char *p, Boolean sel);

/* xec_Toggle.c */
void xec_SetToggle(Widget w, int set);
int xec_GetToggle(Widget w);

/* xec_Widget.c */
void *xec_GetUserData(Widget w);
void xec_SetUserData(Widget w, void *p);
void xec_SetColor(Widget w, Pixel p, const char *which);
void xec_ShowWidget(Widget w);
void xec_Invert(Widget w);
void xec_ManageAll(Widget w);
void xec_UnmanageAll(Widget w);
