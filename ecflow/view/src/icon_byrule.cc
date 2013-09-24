#include "pixmap.h"
/* XPM */
static const char * bits[] = {
/* columns rows colors chars-per-pixel */
"16 16 14 1",
"  c #000080800000",
". c #3196965F3196",
"X c #3E359BF03E35",
"o c #4FC7A3AF4FC7",
"O c #82B4BA2682B4",
"+ c #84AEBB0584AE",
"@ c #9B2CC4F09B2C",
"# c #9EC3C6869EC3",
"$ c #CDA2DB32CDA2",
"% c #D3DDDDF1D3DD",
"& c #D778DF88D778",
"* c #D8EFE02DD8EF",
"= c #E331E4B4E331",
"- c #E5E5E5E5E5E5",
/* pixels */
"       #--------",
"       O--------",
"       @--------",
"       $--------",
"      o---------",
"      *-  X.----",
"    o*--  @%----",
"@+@$----  &-----",
"--------  =-----",
"--------  ------",
"--------  ------",
"--------  ------",
"----------------",
"----------------",
"----------------",
"----------------"
};
static pixmap p("byrule",(const char**)bits);
