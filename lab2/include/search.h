//This file contains function declarations of "search.c"

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

extern char *select_search_string ();

extern int select_field_to_search_by (int*, char**);

extern int match_by_name_or_company (char*, int);

extern int match_link (Ptr_Rolo_List, int, char*, int, char*, int);

extern int find_all_matches (int, char*, char*, Ptr_Rolo_List*);

extern void rolo_search_mode (int , char*, char*);

