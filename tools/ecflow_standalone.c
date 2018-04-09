/*
## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.
*
*  Read stdin and create a session from it
*
*  Read standard in, to a input file, forks a child process as a separate session
*  and then run the shell using the input file.  In the child process, we close
*  standard out, and duplicate standard out as standard error. The next fopen()
*  is then used as standard out/error. i.e output file.
*
*  test using:
*  echo "xxx=\"hello worlds from $HOME\"\nfred=$USER" | ./ecflow_standalone -i in.txt -o out.txt
*
*  in.txt
*  xxx="hello worlds from /home/ma/ma0"
*  fred=ma0
*
*  out.txt
*  + xxx='hello worlds from /home/ma/ma0'
*  + fred=ma0
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

char *nameof(char *name) {
  char *s;
  int len = strlen(name);
  for( s=name+len-1 ; len && *s != '/' ; len-- )  s--;
  if(*s == '/') s++;
  return s;
}

int main(argc,argv) int argc; char **argv;
{
  char *infile = NULL;             /* Temporary input file        */
  char *outfile= "/dev/null";      /* Output file (def /dev/null) */
  char *shell= "/bin/sh";          /* default shell */
  /* int   keep_file=FALSE;*/      /* Flag to keep the input file */

  FILE *fp;                        /* Temp to write the input file */
  char  buff[MAXLEN];              /* Temp buffer to read in lines */

  int   option;
  extern char *optarg;             /* Needed for the getopt */
  extern int   optind;
      
  signal(SIGCHLD,SIG_IGN);

  while( (option=getopt(argc,argv,"i:o:s:")) != -1 )
    switch( option ) {
      case 'i':
        infile = optarg;
        /* keep_file = TRUE; */
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

  /* Copy standard input to infile */
  if( !infile ) infile=(char *)tmpnam(NULL);
  if( !(fp=fopen(infile,"w")) ) {
     perror("STANDALONE-INPUT-FILE");
     exit(1);
  }
  while( fgets( buff,MAXLEN-1,stdin) )  fputs(buff,fp);
  fclose(fp);


  /* fork child process, closing the parent */
  switch(fork()) {
    case -1: perror(*argv); exit(1);
    case  0: break;                       /* child */
    default: exit(0);                     /* The parent exits */
  }


  /* close standard out,and make standard out a copy of standard error
   * This is done so that very next fopen(), will be used as stdout/srderr for execl
   *
   * int dup2(int oldfd, int newfd);
   * makes newfd be the copy of oldfd, closing newfd first if necessary
   * */
  close(2);                        /* close standard error in child */
  dup2(2,1);
  FILE* fout = fopen(outfile,"w"); /* Open file for output and error, when running execl(..) */
  close(0);                        /* close standard in , in child */


  /* make sure infile exists and is readable */
  if( !(fp=fopen(infile,"r")) ) {
    perror("STANDALONE-INPUT-FILE-FOR-SHELL");
    exit(1);
  }
  fclose(fp);

  /* if( !keep_file ) unlink(infile); 
     for (n=3; n<65535 ;n++) fclose(n); */

  /* create a new session from the child process */
#if defined(linux) || defined (hpux) || defined (solaris) || defined (SGI) || defined (SVR4) || defined (AIX) || defined(SYG) || defined(alpha)
  if( setsid() == -1 )
#else
  if( setsid(0) == -1 )
#endif
  {
    perror("STANDALONE-SETSID");
    exit(1);
  }

  execl(shell,nameof(shell),"-x",infile,NULL);
  /* if( !keep_file ) unlink(infile); */

  fclose(fout); /* must be closed last */
  exit(1);
}
