#include <criterion/criterion.h>
#include <criterion/logging.h>
#include "const.h"

Test(basecode_tests_suite, validargs_help_test) {
    int argc = 2;
    char *argv[] = {"bin/transplant", "-h", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit (0x1) not set for -h. Got: %x", opt);
}

Test(basecode_tests_suite, validargs_serialize_test) {
    int argc = 2;
    char *argv[] = {"bin/transplant", "-s", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert(opt & flag, "Compress mode bit wasn't set. Got: %x", opt);
}

Test(basecode_tests_suite, validargs_error_test) {
    int argc = 3;
    char *argv[] = {"bin/transplant", "-s", "-c", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

Test(basecode_tests_suite, help_system_test) {
    char *cmd = "bin/transplant -h";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
}
