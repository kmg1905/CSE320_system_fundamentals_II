//This file contains function declarations of "options.c"

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

extern void print_short ();

/* print the names and phone numbers of everyone in the rolodex. */

extern int person_match (char *person, Ptr_Rolo_Entry entry);

/* Match against a rolodex entry's Name and Company fields. */
/* Thus if I say 'rolo CCA' I will find people who work at CCA. */
/* This is good because sometimes you will forget a name but remember */
/* the company the person works for. */

extern int find_all_person_matches (char *person);


extern void look_for_person (char *person);

extern void print_people ();

extern void interactive_rolo ();