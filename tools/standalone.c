/* This software is provided under the ECMWF standard software license agreement. */
/**************************************************************************
.TITLE   C-SMS-CDP
.NAME    STANDALONE
.SECTION L
.AUTHOR  Otto Pesonen
.DATE    17-DEC-1992 / 09-OCT-1992 / OP
.FILE    standalone.c
.ORIGIN  mandd
*
*  Read stdin and create a session from it
*
************************************o*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef SYG
#define <unistd.h>
#endif

       #include <sys/types.h>
       #include <unistd.h>
       #include <string.h>

#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

#define MAXLEN 1024                /* Maximum line lenght */

char *nameof(char *name)
{
  char *s;

  int len = strlen(name);

  for( s=name+len-1 ; len && *s != '/' ; len-- )
    s--;

  if(*s == '/') s++;

  return s;
}

int main(argc,argv) int argc; char **argv;
{
  char *infile;                    /* Temporary input file        */
  char *outfile;                   /* Output file (def /dev/null) */
  char *shell;
  int   keep_file=FALSE;           /* Flag to keep the input file */

  FILE *fp;                        /* Temp to write the input file */
  char  buff[MAXLEN];              /* Temp buffer to read in lines */

  int   option;
  int   n;

  extern char *optarg;             /* Needed for the getopt */
  extern int   optind;
      
  infile  = NULL;
  outfile = "/dev/null";
  shell   = "/bin/sh";

  signal(SIGCHLD,SIG_IGN);

  while( (option=getopt(argc,argv,"i:o:s:")) != -1 )
    switch( option )
    {
      case 'i':
        infile = optarg;
        keep_file = TRUE;
        break;

      case 'o':
        outfile = optarg;
        break;

      case 's':
        shell = optarg;
        break;

      default:
        fprintf(stderr,"usage: %s [-i inputfile] -o [outputfile]\n",*argv);
        exit(0);
    }

  if( !infile ) { infile=(char *)tmpnam(NULL);

  if( !(fp=fopen(infile,"w")) )
  {
    perror("STANDALONE-INPUT-FILE");
    exit(1);
  }}

  while( fgets( buff,MAXLEN-1,stdin) )
    fputs(buff,fp);

  fclose(fp);

  switch(fork())
  {
    case -1:
      perror(*argv); exit(1);
    case  0:
      break;
    default:
      exit(0);                     /* Jeps! The parent exits */
  }

  close(2);
  FILE* fout = fopen(outfile,"w");
  dup2(2,1);
  close(0);

  if( ! fopen(infile,"r") )
  {
    perror("STANDALONE-INPUT-FILE-FOR-SHELL");
    exit(1);
  }

  /* if( !keep_file ) unlink(infile); 
     for (n=3; n<65535 ;n++) fclose(n); */

#if defined(linux) || defined (hpux) || defined (solaris) || defined (SGI) || defined (SVR4) || defined (AIX) || defined(SYG) || defined(alpha)
  if( setsid() == -1 )
#else
  if( setsid(0) == -1 )
#endif
  {
    perror("STANDALONE-SETSID");
    exit(1);
  }

  execl(shell,nameof(shell), "-x",infile,NULL);
  /* if( !keep_file ) unlink(infile);
  fclose(fout); */
  exit(1);
}

