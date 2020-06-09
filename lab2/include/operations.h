//This file contains function declarations of "operations.c"

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

extern int rolo_add ();
//extern char *ctime1(long*);
//extern char time_stamp[20];

extern int entry_action (Ptr_Rolo_List rlink);

extern void display_list_of_entries  (Ptr_Rolo_List rlist);

extern void rolo_peruse_mode (Ptr_Rolo_List first_rlink);