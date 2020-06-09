/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */

/*
 * This file was not in the original sources.
 * It is included for conformance to the requirement (motivated by the
 * use of Criterion for testing) that the main function should occur alone
 * in its own source file, so that it can be easily substituted with the
 * unit-test-driver main function from the Criterion library to create
 * the test executable.
 */

#include <stdlib.h>

extern int rolo_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    return rolo_main(argc, argv);
}
