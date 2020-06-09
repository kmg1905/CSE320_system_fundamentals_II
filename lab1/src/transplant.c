#include "const.h"
#include "transplant.h"
#include "debug.h"
#include <sys/stat.h>
#include <stdlib.h>

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif


/*
 * A function that returns printable names for the record types, for use in
 * generating debugging printout.
 */
static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
    return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
    return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
    return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
    return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
    return "DIRECTORY_ENTRY";
    case FILE_DATA:
    return "FILE_DATA";
    default:
    return "UNKNOWN";
    }
}

/**
 *@brief These are the definations of functions
 * which are used later in transplant program
 */
int string_compare (char *string1, char *string2);
int string_length (char *string1);
int string_copy(char *string1, char *string2);
int string_concat (char* string1, char* string2);

/**
  *@Brief This function prints the magic sequence
 */
void magic_sequence()
{
    printf("%c", MAGIC0);
    printf("%c", MAGIC1);
    printf("%c", MAGIC2);
}


/*
 *@brief  This function prints byte representation of given integer data
 * in Big endian format
 */
void int_to_byte(int data)
{
    unsigned char a,b,c,d;
    a =(data & 0xFF);
    b=(data >> 8 & 0xFF);
    c=(data >> 16 & 0xFF);
    d=(data >> 24 & 0xFF);

    printf("%c", d);
    printf("%c", c);
    printf("%c", b);
    printf("%c", a);
}

/*
 *brief   This function converts given long data into it's byte representation
 * inbig endian format
 */
void long_to_byte(long data)
{
    unsigned char a,b,c,d,e,f,g,h;
    a =(data & 0xFF);
    b=(data >> 8 & 0xFF);
    c=(data >> 16 & 0xFF);
    d=(data >> 24 & 0xFF);
    e=(data >> 32 & 0xFF);
    f=(data >> 40 & 0xFF);
    g=(data >> 48 & 0xFF);
    h=(data >> 56 & 0xFF);

    printf("%c", h);
    printf("%c", g);
    printf("%c", f);
    printf("%c", e);
    printf("%c", d);
    printf("%c", c);
    printf("%c", b);
    printf("%c", a);
}

/*
 * @breif This function checks whether the three characters read are
 * part of magic sequence or not
 */
int magic_sequence_check()
{
    if (getchar() != 0x0c || getchar() != 0x0d || getchar() != 0xed)
        return -1;
    else
        return 0;
}

/*
 * @brief This function takes a single character and appends it to the path_buf variable
 */
void dir_name_concat(char ch)
{
    int i;
    for (i=0; *(path_buf + i) != '\0'; i++);
    *(path_buf + i) = ch;
    *(path_buf + i + 1) = '\0';
    path_length++;

}

/*
 * @brief This function converts four bytes of char data into an integer
 */
int byte_to_int()
{
    int result =  (int)getchar()  << 24| (int)getchar() << 16 | (int)getchar() << 8 |(int)getchar();
    return result;
}

/*
 * @brief This function converts eight consecutive bytes to long variable
 */
long byte_to_long()
{
    long result =  (long)getchar() << 56 | (long)getchar() << 48 | (long)getchar() << 40 |
    (long)getchar() << 32 | (long)getchar() << 24 | (long)getchar() << 16 |
    (long)getchar() << 8 | (long)getchar();
    return result;
}

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    int length = string_length(name);
    //printf("%d \n%d", NAME_MAX, PATH_MAX);
    if (length < NAME_MAX && length < PATH_MAX)
    {
        if (!string_copy(name, name_buf))
        {
            if (!string_copy(name, path_buf))
            {
                path_length = length;
                return 0;
            }
        }

    }
    return -1;
}

/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 *
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {

 path_length += string_length(name);

    if (PATH_MAX > path_length)
    {
        string_concat(path_buf, "/");
        string_concat(path_buf, name);
        return 0;
    }
    return -1;
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop()
{
    int i;
    for ( i = path_length - 1; *(path_buf + i) != '/'; i --)
    {
        if (i)
            *(path_buf + i) = '\0';
    }

    if (i > 2)
    {
        *(path_buf + i) = '\0';
        i--;
        path_length = i;
        return 0;
    }
    return -1;
}

/* @brief This function prints the file name read from path_buf
 */
void file_name()
{
    int i,j, length = string_length(path_buf);
    for (i = 0; i < length; i++)
        if (*(path_buf + i) == '/')
            j = i;

    for (i = j+1; i < length; i++)
        printf("%c", *(path_buf + i));
}

/* @brief This function determines the length of filename in path_buf
 */
int file_name_length()
{
    int i,j, length = string_length(path_buf);
    for (i = 0; i < length; i++)
        if (*(path_buf + i) == '/')
            j = i;
    return length - j - 1;
}

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth)
{
    mkdir (path_buf, 0700);
    byte_to_long();
    while(1)
    {
        if (magic_sequence_check())
            return -1;

        int type1 = getchar();
        if (type1 == END_OF_DIRECTORY)
        {
            return 0;
        }
        int depth = byte_to_int();
        long total_size = byte_to_long();
        int dir_name_length = total_size - 28;
        byte_to_int();
        byte_to_long();

        dir_name_concat('/');
        while(dir_name_length != 0)
        {

            dir_name_concat(getchar());
            -- dir_name_length;
        }

        if (magic_sequence_check())
            return -1;

        int type2 = getchar();
        byte_to_int();

        if (type1 == DIRECTORY_ENTRY && type2 == START_OF_DIRECTORY)
        {
            if (deserialize_directory(depth))
                return -1;
        }
        else if (type1 == DIRECTORY_ENTRY && type2 == FILE_DATA)
        {
            if (deserialize_file(depth))
                return -1;
        }
        path_pop();

    }

    return 0;

}

/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth)
{
    long size = byte_to_long();

    FILE *fptr;

    fptr = fopen(path_buf, "r");

    if (fptr != NULL)
    {
        if (global_options & 8)
        {
            ;
        }
        else
        {
            printf("File already exists\n");
            return -1;
        }

    fclose(fptr);
    }


    fptr = fopen(path_buf,"w");

    if (fptr == NULL)
    {
        printf("Unable to create the file\n");
        return -1;
    }

    char ch;
    size = size - 16;
    for (int i = 0; i < size; i++)
    {
        ch = getchar();
        fputc(ch, fptr);
    }
    fclose(fptr);
    return 0;
}

/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int serialize_directory(int depth)
{
    DIR *directory;
    directory = opendir(path_buf);

    if (directory == NULL)
    {
        printf("Invalid directory path\n");
        return -1;
    }

    //DIRECTORY ENTRY
    magic_sequence();
    printf("%c", DIRECTORY_ENTRY);
    int_to_byte(depth);

    struct dirent *entry;
    struct stat stat_data;
    stat(path_buf, &stat_data);

    int mode = stat_data.st_mode;
    long size = stat_data.st_size;
    int file_length = file_name_length();
    long total_size = 28 + file_length;

    long_to_byte(total_size);
    int_to_byte(mode);
    long_to_byte(size);
    file_name();

    //START OF DIRECTORY
    magic_sequence();
    printf("%c", START_OF_DIRECTORY);

    depth++;
    int_to_byte(depth);
    size = 16;
    long_to_byte(size);

    while((entry = readdir(directory)) != NULL)
    {
        if (!string_compare(entry -> d_name, "." ) || !string_compare(entry -> d_name, ".."));

        else
        {
            path_push(entry -> d_name);
            stat(path_buf, &stat_data);

            if (S_ISDIR(stat_data.st_mode))
            {
                if (serialize_directory(depth + 1))
                    return -1;
            }
            else if(S_ISREG(stat_data.st_mode))
            {
                if (serialize_file(depth, stat_data.st_size))
                    return -1;
            }

            path_pop();
        }
    }

    magic_sequence();
    printf("%c", END_OF_DIRECTORY);
    int_to_byte(depth);
    long_to_byte(16);

    closedir(directory);
    return 0;
}

/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size)
{
    FILE *fptr;
    fptr = fopen(path_buf, "r");

    if (fptr == NULL)
    {
        printf("cannot open file\n");
        return -1;
    }

    magic_sequence();
    printf("%c", DIRECTORY_ENTRY);

    int_to_byte(depth);
    int file_length = file_name_length();
    long total_size = 28 + file_length;
    long_to_byte(total_size);

    struct stat stat_data;
    stat(path_buf, &stat_data);
    int mode = stat_data.st_mode;
    int_to_byte(mode);
    long_to_byte(size);
    file_name();

    //FILE DATA
    magic_sequence();
    printf("%c", FILE_DATA);

    int_to_byte(depth);
    size =stat_data.st_size;
    size += 16;
    long_to_byte(size);

    char ch = fgetc(fptr);

    while (ch != EOF)
    {
        putchar(ch);
        ch = fgetc(fptr);

    }

    fclose(fptr);
    return 0;
}

/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize()
{

    DIR *directory;
    directory = opendir(path_buf);

    if (directory == NULL)
    {
        printf("Invalid directory path\n");
        return -1;
    }

    //SOT
    magic_sequence();
    int depth = 0;
    long size = 16;

    printf("%c", START_OF_TRANSMISSION);
    int_to_byte(depth);
    long_to_byte(size);

    //SOD
    magic_sequence();
    depth ++;
    printf("%c", START_OF_DIRECTORY);
    //printf("%d", depth);
    int_to_byte(depth);

    struct dirent *entry;
    struct stat stat_data;

    size = 16;
    long_to_byte(size);

    while((entry = readdir(directory)) != NULL)
    {
        if (!string_compare(entry -> d_name, "." ) || !string_compare(entry -> d_name, ".."));

        else
        {
            path_push(entry -> d_name);
            stat(path_buf, &stat_data);

            if (S_ISDIR(stat_data.st_mode))
            {
                if (serialize_directory(depth))
                    return -1;
            }
            else if(S_ISREG(stat_data.st_mode))
            {
                if (serialize_file(depth, stat_data.st_size))
                    return -1;
            }

            path_pop();
        }
    }

    // END OF DIRECTORY
    magic_sequence();

    depth = 1;
    printf("%c", END_OF_DIRECTORY);
    int_to_byte(depth);
    long_to_byte(size);

    // End OF TRANSMISSION
    magic_sequence();

    printf("%c", END_OF_TRANSMISSION);
    depth = 0;
    size = 16;
    int_to_byte(depth);
    long_to_byte(size);

    closedir(directory);
    return 0;
}
/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */
int deserialize()
{
    //SOT byte data
     if (magic_sequence_check())
        return -1;

    getchar();
    byte_to_int();
    byte_to_long();

    //SOD byte data

    if (magic_sequence_check())
            return -1;
    getchar();
    byte_to_int();
    byte_to_long();

    mkdir(path_buf, 0700);

    //Directory Entry
    while(1)
    {

        if (magic_sequence_check())
            return -1;

        int type1 = getchar();
        if (type1 == END_OF_DIRECTORY )
            break;
        int depth = byte_to_int();
        long total_size = byte_to_long();
        int dir_name_length = total_size - 28;
        byte_to_int();
        byte_to_long();

        dir_name_concat('/');
        while ((dir_name_length != 0))
        {

            dir_name_concat(getchar());
            dir_name_length--;
        }

        if (magic_sequence_check())
            return -1;

        int type2 = getchar();
        byte_to_int();

        if (type1 == DIRECTORY_ENTRY && type2 == START_OF_DIRECTORY)
        {
            if (!deserialize_directory(depth))
            {
                byte_to_int();
                byte_to_long();
            }
        }
        else if (type1 == DIRECTORY_ENTRY && type2 == FILE_DATA)
        {
            if (deserialize_file(depth))
                return -1;
        }
        path_pop();

    }

    while(getchar());

    return 0;
}


/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    if (argc > 1 && argc < 7)
    {
        if (!string_compare(*(argv + 1) , "-h"))
        {
            global_options = global_options | 1;
            return 0;
        }
        else if(!string_compare(*(argv + 1) , "-s"))
        {
            if (argc > 2)
            {

                if (!string_compare(*(argv + 2), "-c"))
                    return -1;
                else if (!string_compare(*(argv + 2), "-p"))
                {
                    if (argc == 3)
                        return -1;
                    else
                    {
                        if (path_init( *(argv + 3) ))
                            return -1;
                        if (!string_compare(*(argv + 3), "-c"))
                            return -1;
                    }
                    if (argc == 5)
                        if (!string_compare(*(argv + 4), "-c"))
                            return -1;
                }
                else
                    return -1;
            }
            global_options = global_options | 2;

            if (argc == 2 && path_init("."))
                return -1;
            return 0;
        }
        else if(!string_compare(*(argv + 1) , "-d"))
        {
            if (argc > 2)
            {
                if (!string_compare(*(argv + 2), "-p"))
                {
                    if (argc == 3)
                        return -1;
                    else
                    {
                        if (!string_compare(*(argv +3), "-c"))
                            return -1;

                        if (path_init(*(argv +3)))
                            return -1;
                    }

                    if (argc == 5)
                    {
                        if (!string_compare(*(argv +4), "-c"))
                        {
                            global_options = global_options | 8;
                        }
                    }
                }
                else
                    return -1;
            }
            global_options = global_options | 4;
            if (argc == 2 && path_init("."))
                return -1;
            return 0;
        }
    }
return -1;
}

/**
  *@Brief This function compares two strings and returns the difference between them
  */
int string_compare (char *string1, char *string2) {
    int string1_length = string_length(string1);
    int string2_length = string_length(string2);
    if (string1_length != string2_length)
        return -1;
    for (int i = 0; i < string1_length; i++) {

        if (*(string1 + i) != *(string2 +i))
            return -1;
    }
    return 0;

}

/**
  *@Brief This function computes the length of the given string and returns it's value
  */
int string_length (char *string1) {
    int i;
    for (i = 0; *(string1 + i) != '\0'; i++){}
    return i;

}

/**
  *@Brief This function is used to copy the contents of first funtion
  *into the second function
  */

int string_copy(char *string1, char *string2)
{
    int length = string_length(string1);
    for (int i = 0; i <= length; i++)
    {
        *(string2 + i) = *(string1 +i);
    }
    return 0;
}

/**
  *@Brief This function concats two strings and returns the resulted string
  */

int string_concat (char* string1, char* string2)
{
    int i =0, j = 0;
    for ( i = string_length(string1) , j = 0; *(string2 +j) != '\0'; j++ )
    {

        *(string1 + i) = *(string2 + j);
        i += 1;
    }
    *(string1 + i ) = '\0';
    return 0;
}
