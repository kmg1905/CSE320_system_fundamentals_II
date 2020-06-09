#include <stdio.h>
#include <ctype.h>

#include "sys5.h"

#ifdef TMC
#include <ctools.h>
#else
#include "ctools.h"
#endif
#include "args.h"
#include "menu.h"
#include  "mem.h"

#include "rolofiles.h"
#include "rolodefs.h"
#include "datadef.h"
#include "menuaux.h"
#include "io.h"
#include "choices.h"
#include "clear.h"
#include "operations.h"
#include "rolo.h"



int rolo_menu_yes_no (char *prompt, int rtn_default, int help_allowed, char *helpfile, char *subject)

{
  int rval;
  reask :
  rval = menu_yes_no_abort_or_help (
              prompt,ABORTSTRING,help_allowed,rtn_default
           );
  switch (rval)
  {
    case MENU_EOF :
      user_eof();
      break;
    case MENU_HELP :
      cathelpfile(libdir(helpfile),subject,1);
      goto reask;
      break;
    default :
      return(rval);
      break;
  }
  return 0;
}


int rolo_menu_data_help_or_abort (char *prompt, char *helpfile, char *subject, char **ptr_response)

{
  int rval;
  reask :
  rval = menu_data_help_or_abort(prompt,ABORTSTRING,ptr_response);
  if (rval == MENU_HELP)
  {
     cathelpfile(libdir(helpfile),subject,1);
     goto reask;
  }
  else if (rval == MENU_EOF)
    return MENU_EOF;

  return(rval);
}


int rolo_menu_number_help_or_abort (char *prompt, int low, int high, int *ptr_ival)

{
  int rval;
  if (MENU_EOF == (rval = menu_number_help_or_abort (
                               prompt,ABORTSTRING,low,high,ptr_ival
                           )))
     user_eof();
  return(rval);
}
