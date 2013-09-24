#include "pixmap.h"
/* XPM */
static const char * bits[] = {
/* columns rows colors chars-per-pixel */
"16 16 15 1",
"  c #FFFFA5A50000",
". c #FDA4AB7114BC",
"X c #FC6AAE771F8E",
"o c #F6F0BBF14FC7",
"O c #F488C1DD64F9",
"+ c #F0EFCABA84AE",
"@ c #F128CA2C82B4",
"# c #EE61D1039B2C",
"$ c #EDFAD2019EB8",
"% c #EBA0D7C9B368",
"& c #E8A6DF1DCDA2",
"* c #E80BE09AD2F6",
"= c #E75EE246D8EF",
"- c #E64EE4E3E24A",
"; c #E5E5E5E5E5E5",
/* pixels */
"       $;;;;;;;;",
"       @;;;;;;;;",
"       #;;;;;;;;",
"       &;;;;;;;;",
"      o;;;;;;;;;",
"      =;  ;;  ;;",
"    o=;;  ;;  ;;",
"#+#&;;;;  ;;  ;;",
";;;;;;;;  ;;  ;;",
";;;;;;;;  -*  ;;",
";;;;;;;;X %O  ;;",
";;;;;;;;$.X%  ;;",
";;;;;;;;;;;;;;;;",
";;;;;;;;;;;;;;;;",
";;;;;;;;;;;;;;;;",
";;;;;;;;;;;;;;;;"
};
static pixmap p("force_abort",(const char**)bits);
