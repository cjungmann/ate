#ifndef ATE_UTILITIES_H
#define ATE_UTILITIES_H

#include <builtins.h>
// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include "ate_handle.h"

bool get_string_from_list(const char **result, WORD_LIST *list, int index);
bool get_int_from_list(int *result, WORD_LIST *list, int index);

void ate_dispose_variable_value(SHELL_VAR *var);

int get_handle_from_name(AHEAD **head, const char *name_handle);

int get_shell_var_by_name_and_type(SHELL_VAR **retval,
                                   const char *name,
                                   int attributes);




#endif
