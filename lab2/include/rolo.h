//This file contains function declarations of "rolo.c"

#ifndef Bool
#define Bool int
#endif

#ifndef T
#define T 1
#endif

#ifndef F
#define F 0
#endif

#ifndef MAXINT
#define MAXINT 2147483647
#define MAXINTSTR "2147483647"
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 80
#endif


/* if the user hits C-D */
extern void user_eof ();

extern void exit_custom (int);

extern void roloexit (int );

extern void save_and_exit(int );

extern char *rolo_emalloc (int );

/* memory allocating string copy routine */
extern char *copystr (char* );


extern char *timestring ();

extern void user_interrupt ();

extern void save_to_disk ();

extern char *home_directory (char* );

extern char *homedir (char* );

extern char *libdir (char*);

extern Bool rolo_only_to_read ();

extern void locked_action ();


