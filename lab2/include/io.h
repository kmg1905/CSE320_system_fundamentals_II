//This header contains code for io.c file

extern int read_rolodex (int fd);

extern void write_rolo_list (FILE *fp);

extern void write_rolo (FILE *fp1, FILE *fp2);

extern void display_basic_field (char *name, char *value, int show, int up);

extern void display_other_field (char *fieldstring);

extern void summarize_entry_list (Ptr_Rolo_List rlist, char *ss);

extern void display_field_names ();

extern void display_entry (Ptr_Rolo_Entry entry);

extern void display_entry_for_update (Ptr_Rolo_Entry entry);

extern int cathelpfile (char *filepath, char *helptopic, int clear);

extern int any_char_to_continue ();