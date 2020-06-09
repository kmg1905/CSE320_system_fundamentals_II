/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef TRANSPLANT_H
#define TRANSPLANT_H

/*
 * The serialized data produced by "transplant" consists of a sequence
 * of records.  Each record starts with a fixed-format header, which
 * specifies the type of the record and the total length of the record
 * including the header.  Some types of records consist only of a
 * header.  Other types contain data that follows the header.
 *
 * Record headers consist of 16 bytes of data, with the following format:
 *
 *   Magic (3 bytes): 0x0C, 0x0D, 0xED
 *   Type (1 byte)
 *   Depth (4 bytes)
 *   Size (8 bytes)
 *
 * The first three bytes are the "magic sequence" 0x0C, 0x0D, 0xED.
 * The presence of this sequence in each record helps a program reading
 * the serialized data to detect if it has been corrupted.
 * Following the magic sequence is a single byte that specifies the type
 * of the record.  Following the type are four bytes that specify the depth
 * in the tree for this record, as detailed below.  The depth is specified
 * as an unsigned 32-bit integer in big-endian format.
 * Following the type are 8 bytes that specify the size of the record as
 * an unsigned 64-bit integer in big-endian format.  The size of the
 * record is the total number of bytes comprising the record, including
 * the header bytes as well as any additional data after the header.
 */
 
#define HEADER_SIZE 16

#define MAGIC0 0x0C
#define MAGIC1 0x0D
#define MAGIC2 0xED

/*
 * The possible values of the type field are given by the following defines:
 */
#define START_OF_TRANSMISSION 0
#define END_OF_TRANSMISSION 1
#define START_OF_DIRECTORY 2
#define END_OF_DIRECTORY 3
#define DIRECTORY_ENTRY 4
#define FILE_DATA 5
#define NUM_RECORD_TYPES 5

/*
 * Serialized data produced by "transplant" always begins with a single
 * START_OF_TRANSMISSION record and ends with a single END_OF_TRANSMISSION record.
 * These records consist only of a header (and so their size is 16 bytes).
 * The depth fields of these records contain the value 0.
 *
 * A START_OF_DIRECTORY record indicates the beginning of data corresponding to
 * a subdirectory and an END_OF_DIRECTORY entry indicates the end of data
 * corresponding to a subdirectory.  These records also consists only of a header.
 * A START_OF_DIRECTORY record has a depth that is one greater than that of
 * the record that immediately precedes it.  The corresponding subdirectory data
 * comprises the consecutive sequence of records following the START_OF_DIRECTORY
 * record which have a depth greater than or equal to that of the START_OF_DIRECTORY
 * record.  This sequence of records is required to be terminated by an END_OF_DIRECTORY
 * record with the same depth, so that START_OF_DIRECTORY and END_OF_DIRECTORY records
 * of each depth always occur in matched pairs, like parentheses.
 *
 * A DIRECTORY_ENTRY record specifies the name of an entry within the current
 * directory, along with metadata associated with that entry.
 * There are DIRECTORY_ENTRY records both for regular files and for subdirectories.
 * The metadata, which has a fixed length, occurs immediately following the
 * header.  Following the metadata is a file name, which can be of varying lengths.
 * File names consist simply of a sequence of arbitrary bytes (with no null terminator).
 * The length of the name can be determined by subtracting from the total size of the
 * record the size of the header and the size of the metadata.
 *
 * A FILE_DATA record specifies the content of the file whose name is given
 * by the immediately preceding DIRECTORY_ENTRY record.  The content consists
 * of the sequence of bytes of data immediately following the FILE_DATA header.
 * The length of this sequence of bytes can be determined by subtracting the size
 * of the record header from the total size of the record.
 */

/*
 * The metadata contained in a DIRECTORY_ENTRY record consists of the following:
 *
 *   4 bytes of type/permission information (type "mode_t", in big-endian
 *     format), as specifed for the "st_mode" field of the "struct stat" structure
 *     in the man page for the stat(2) system call.
 *   8 bytes of size information (type "off_t", in big-endian format),
 *     as specified for the "st_size" field of the "struct stat" structure.
 */

#endif
