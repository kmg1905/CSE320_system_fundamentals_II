//This header file contains declarations of rlist.c

int rlength (Ptr_Rolo_List rlist);

Ptr_Rolo_List new_link_with_entry ();

int rolo_insert (Ptr_Rolo_List link, int (*compare)());

int rolo_delete (Ptr_Rolo_List link);

int compare_links (Ptr_Rolo_List l1, Ptr_Rolo_List l2);

void rolo_reorder ();