#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"



#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

#define exit 1
#define serialize 2
#define deserialize 4 

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & exit)
        USAGE(*argv, EXIT_SUCCESS);
    if(global_options & serialize)
    {
        if (serialize())
            return EXIT_FAILURE;
    }
    if(global_options & deserialize)
    {
        if (deserialize())
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
