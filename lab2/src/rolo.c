#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <signal.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sys5.h"

#ifdef TMC
#include <ctools.h>
#else
#include "ctools.h"
#endif
#include "args.h"
#include "menu.h"
#include "mem.h"

#include "rolofiles.h"
#include "rolodefs.h"
#include "datadef.h"
#include "options.h"
#include "clear.h"
#include "choices.h"
#include "io.h"
#include "rolo.h"
#include "operations.h"
#include "search.h"
#include "menuaux.h"
#include "update.h"
#include "rlist.h"


static char rolodir[DIRPATHLEN];        /* directory where rolo data is */
static char filebuf[DIRPATHLEN];        /* stores result of homedir() */
extern Ptr_Cmd_Line Cmd;

int changed = 0;
int reorder_file = 0;
int rololocked = 0;
int in_search_mode = 0;


char *rolo_emalloc (int size)

/* error handling memory allocaor */

{
  char *rval;
  if (0 == (rval = malloc(size))) {
     fprintf(stderr,"Fatal error:  out of memory\n");
     save_and_exit(-1);
  }
  return(rval);
}


char *copystr (char *s)

/* memory allocating string copy routine */


{
 char *copy;
 if (s == 0) return(0);
 copy = rolo_emalloc(strlen(s) + 1);
 strcpy(copy,s);
 return(copy);
}


char *timestring ()

/* returns a string timestamp */

{
  char *s;
  long timeval = 0;
  ctime(&timeval);
  s = ctime(&timeval);
  s[strlen(s) - 1] = '\0';
  return(copystr(s));
}


void user_interrupt ()

/* if the user hits C-C (we assume he does it deliberately) */

{
  unlink(homedir(ROLOLOCK));
  fprintf(stderr,"\nAborting rolodex, no changes since last save recorded\n");
  exit(-1);
}


void user_eof ()

/* if the user hits C-D */

{
  unlink(homedir(ROLOLOCK));
  fprintf(stderr,"\n\nUnexpected EOF on terminal. Saving rolodex and exiting\n");
  fflush(stderr);
  fflush(stdout);
  save_and_exit(-1);
}


void roloexit (int rval)
{
  if (rololocked) unlink(homedir(ROLOLOCK));
  //free(ptr_space);
  exit(rval);
}



void save_to_disk ()

/* move the old rolodex to a backup, and write out the new rolodex and */
/* a copy of the new rolodex (just for safety) */

{
  FILE *tempfp,*copyfp;
  char d1[DIRPATHLEN], d2[DIRPATHLEN];
  //int r;

  tempfp = fopen(homedir(ROLOTEMP),"w");
  copyfp = fopen(homedir(ROLOCOPY),"w");
  if (tempfp == NULL || copyfp == NULL)
  {
     fprintf(stderr,"Unable to write rolodex...\n");
     fprintf(stderr,"Any changes made have not been recorded\n");
     roloexit(-1);
  }
  write_rolo(tempfp,copyfp);

  if (fclose(tempfp) == -1)
    roloexit(-1);
  if (fclose(copyfp) == -1)
    roloexit(-1);
  if (rename(strcpy(d1,homedir(ROLODATA)),strcpy(d2,homedir(ROLOBAK))) ||
      rename(strcpy(d1,homedir(ROLOTEMP)),strcpy(d2,homedir(ROLODATA))))
  {
     fprintf(stderr,"Rename failed.  Revised rolodex is in %s\n",ROLOCOPY);
     roloexit(-1);
  }
  printf("Rolodex saved\n");
  sleep(1);
  changed = 0;
}


void save_and_exit(int rval)
{
  if (changed) save_to_disk();
  roloexit(rval);
}


extern struct passwd *getpwnam();

char *home_directory (char *name)
{
  struct passwd *pwentry;
  if (0 == (pwentry = getpwnam(name))) return("");
  return(pwentry -> pw_dir);
}


char *homedir (char *filename)

/* e.g., given "rolodex.dat", create "/u/massar/rolodex.dat" */
/* rolodir generally the user's home directory but could be someone else's */
/* home directory if the -u option is used. */

{
  char ch[] = "\0";
  char ch2[] = "/";
  nbuffconcat(filebuf,3,rolodir,ch2,filename, ch, ch, ch);
  return(filebuf);
}


char *libdir (char *filename)

/* return a full pathname into the rolodex library directory */
/* the string must be copied if it is to be saved! */

{
  char ch[] = "\0";
  char ch2[] = "/";;
  nbuffconcat(filebuf,3,ROLOLIB,ch2,filename,  ch, ch, ch);
  return(filebuf);
}


Bool rolo_only_to_read (int sflag, int i)
{
  return((sflag == 1) || i > 0);
}


void locked_action (int lflag)
{
  if (lflag == 1) {
     fprintf(stderr,"Someone else is modifying that rolodex, sorry\n");
     exit(-1);
  }
  else {
     cathelpfile(libdir("lockinfo"),"locked rolodex",0);
     exit(-1);
  }
}


int rolo_main ( int argc, char *argv[])

{
    int fd,in_use,read_only,rolofd;
    Bool not_own_rolodex = 0;
    char *user;
    FILE *tempfp;

    clearinit();
    clear_the_screen();
    int uflag = 0;
    int sflag = 0;
    int lflag = 0;
    int c,i;
    char **names;

    names = (char**)malloc(50);

  while ((c = getopt(argc, argv, "lsu:")) != -1)
  {

    switch(c)
    {
      case 'u':
      {
        uflag = 1;
        user = optarg;
        break;
      }
      case 's':
      {
        sflag = 1;
        break;
      }
      case 'l':
      {
        lflag = 1;
        break;
      }
      case '?':
      printf("Illegal option \n");
      printf("%s\n", USAGE);
      roloexit(-1);
    }
  }

  for (i=0; optind < argc; optind++)
    {
      names[i] = (char*)malloc(50);
      strcpy(names[i], argv[optind]);
      i++;

    }

    /* find the directory in which the rolodex file we want to use is */

    if (uflag) not_own_rolodex = T;
    if (not_own_rolodex)
    {
       if (user == NIL) {
          fprintf(stderr,"Illegal syntax using -u option\nusage: %s\n",USAGE);
          roloexit(-1);
       }
    }
    else {
       if (0 == (user = getenv("HOME"))) {
          fprintf(stderr,"Cant find your home directory, no HOME\n");
          roloexit(-1);
       }
    }

    if(not_own_rolodex)
    {
       strcpy(rolodir,home_directory(user));
       if (*rolodir == '\0')
       {
          fprintf(stderr,"No user %s is known to the system\n",user);
          roloexit(-1);
       }
       //printf("in if od not own rolodex\n");
    }
    else strcpy(rolodir,user);

    //printf("%d\n", not_own_rolodex);

    /* is the rolodex readable? */

    if (0 != access(homedir(ROLODATA),R_OK))
    {

       /* No.  if it exists and we cant read it, that's an error */

       if (0 == access(homedir(ROLODATA),F_OK))
       {
          fprintf(stderr,"Cant access rolodex data file to read\n");
          roloexit(-1);
       }

       /* if it doesn't exist, should we create one? */

       if (uflag ==1)
       {
          fprintf(stderr,"No rolodex file belonging to %s found\n",user);
          roloexit(-1);
       }

       /* try to create it */

       if (-1 == (fd = creat(homedir(ROLODATA),0644)))
       {
          fprintf(stderr,"couldnt create rolodex in your home directory\n");
          roloexit(-1);
       }

       else
       {
          close(fd);
          fprintf(stderr, "Creating empty rolodex...\n");
          //fprintf(stderr, "Unexpected EOF on terminal. Saving rolodex and exiting\n" );
          fflush(stderr);
       }

    }

    /* see if someone else is using it */

    in_use = (0 == access(homedir(ROLOLOCK),F_OK));

    /* are we going to access the rolodex only for reading? */

    if (!(read_only = rolo_only_to_read(sflag,i)))
    {

       /* No.  Make sure no one else has it locked. */

       if (in_use)
       {
          locked_action(lflag);
       }

       /* create a lock file.  Catch interrupts so that we can remove
       the lock file if the user decides to abort
       */

       if (!(lflag == 1))
       {
          if ((fd = open(homedir(ROLOLOCK),O_EXCL|O_CREAT,00200|00400)) < 0)
          {
             fprintf(stderr,"unable to create lock file...\n");
             exit(1);
          }
          rololocked = 1;
          close(fd);
          signal(SIGINT,user_interrupt);
       }

       /* open a temporary file for writing changes to make sure we can */
       /* write into the directory */

       /* when the rolodex is saved, the old rolodex is moved to */
       /* a '~' file, the temporary is made to be the new rolodex, */
       /* and a copy of the new rolodex is made */

       if (NULL == (tempfp = fopen(homedir(ROLOTEMP),"w")))
       {
           fprintf(stderr,"Can't open temporary file to write to\n");
           roloexit(-1);
       }
       fclose(tempfp);

    }

    allocate_memory_chunk(CHUNKSIZE);

    //printf("%ls\n", (int*)NULL);


    if ((rolofd = open(homedir(ROLODATA),O_RDONLY)) < 0)
    {
        printf("%d\n", rolofd);
        fprintf(stderr,"Can't open rolodex data file to read\n");
        //free(ptr_space);
        roloexit(-1);
    }

    /* read in the rolodex from disk */
    /* It should never be out of order since it is written to disk ordered */
    /* but just in case... */

    if (!read_only) printf("Reading in rolodex from %s\n",homedir(ROLODATA));
    read_rolodex(rolofd);
    close(rolofd);
    if (!read_only) printf( "%d entries listed\n",rlength(Begin_Rlist));
    if (reorder_file && !read_only) {
       fprintf(stderr,"Reordering rolodex...\n");
       rolo_reorder();
       fprintf(stderr,"Saving reordered rolodex to disk...\n");
       save_to_disk();
    }

    /* the following routines live in 'options.c' */

    /* -s option.  Prints a short listing of people and phone numbers to */
    /* standard output */

    if (sflag == 1) {
        print_short();
        exit(0);
    }

    /* rolo <name1> <name2> ... */
    /* print out info about people whose names contain any of the arguments */

    if (i > 0) {
       print_people(names);
       exit(0);
    }

    /* regular rolodex program */

    for (int a =0; a < i; a++)
    {
      free(names[a]);

    }
    free(names);

    interactive_rolo();


    return 0;

}
