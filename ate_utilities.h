#ifndef ATE_UTILITIES_H
#define ATE_UTILITIES_H

#include <builtins.h>
// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include "ate_handle.h"

/**
 * @defgroup ATE_UTILITIES Miscellaneous Functions
 *
 * These functions handle needs that are not specific to any
 * specific action.  Some exists simply to name a code objective to
 * clarify an intention.  Others are here because they are used
 * in multiple places.
 *
 * @{
 */

bool make_unique_name(char *buffer, int bufflen, const char *stem);

bool get_int_from_string(int *result, const char *str);
int set_var_from_int(SHELL_VAR *result, int value);

void ate_dispose_variable_value(SHELL_VAR *var);

int reindex_array_elements(AHEAD *head);

ARRAY_ELEMENT *get_end_of_row(ARRAY_ELEMENT *row, int row_size);

int table_extend_rows(AHEAD *head, int new_columns);
int table_contract_rows(AHEAD *head, int columns_to_remove);

int invoke_shell_function(SHELL_VAR *function, ...);
int invoke_shell_function_word_list(SHELL_VAR *function, WORD_LIST *wl);

/**
 * @defgroup ARG_VARS Functions supporting getting and making SHELL_VAR arguments
 * @{
 */

int create_var_by_stem(SHELL_VAR **var, const char *stem, const char *action);
int create_array_var_by_stem(SHELL_VAR **var, const char *stem, const char *action);

int get_handle_var_by_name_or_fail(SHELL_VAR **rvar,
                                   const char *name,
                                   const char *action);

int create_handle_by_name_or_fail(SHELL_VAR **rvar,
                                  const char *name,
                                  AHEAD *ahead,
                                  const char *action);

int create_var_by_given_or_default_name(SHELL_VAR **rvar,
                                        const char *name,
                                        const char *default_name,
                                        const char *action);

int create_array_var_by_given_or_default_name(SHELL_VAR **rvar,
                                              const char *name,
                                              const char *default_name,
                                              const char *action);

int get_array_var_by_name_or_fail(SHELL_VAR **rvar,
                                  const char *name,
                                  const char *action);


int get_function_by_name_or_fail(SHELL_VAR **rvar,
                                 const char *name,
                                 const char *action);

int update_row_array(SHELL_VAR *target_var, ARRAY_ELEMENT *source_row, int row_size);
/** @} */


/** @} */


#endif
