extern char *ptr_space;

#define NO_MORE_MEMORY -1

extern int allocate_memory_chunk();     /* arg1: int space */

extern char *get_memory_chunk();       /* arg1: int size */

extern char *store_string();           /* arg1: char *str, arg2: int len */
