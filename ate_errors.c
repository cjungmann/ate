#include <builtins.h>
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif
#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>

int ate_error_missing_action(void)
{
   fprintf(stderr, "Missing action name.\n");
   return EX_USAGE;
}

int ate_error_handle_already_exists(const char *handle_name)
{
   fprintf(stderr, "Variable '%s' already exists.\n", handle_name);
   return EX_USAGE;
}

int ate_error_failed_to_make_handle(const char *reason)
{
   fprintf(stderr, "Failed to create new ate handle: %s.\n", reason);
   return EX_MISCERROR;
}

int ate_error_missing_arguments(const char *action_name)
{
   fprintf(stderr, "Missing arguments for action '%s'.\n", action_name);
   return EX_USAGE;
}

int ate_error_arg_not_number(const char *arg_value)
{
   fprintf(stderr, "Argument value '%s' passed instead of number.\n", arg_value);
   return EX_USAGE;
}

int ate_error_not_found(const char *what)
{
   fprintf(stderr, "Failed to find %s.\n", what);
   return EX_NOTFOUND;
}

int ate_error_action_not_found(const char *action_name)
{
   fprintf(stderr, "Failed to find action '%s'.\n", action_name);
   return EX_NOTFOUND;
}

int ate_error_function_not_found(const char *func_name)
{
   fprintf(stderr, "Failed to find funtion '%s'.\n", func_name);
   return EX_NOTFOUND;
}

int ate_error_var_not_found(const char *var_name)
{
   fprintf(stderr, "Failed to find variable '%s'.\n", var_name);
   return EX_NOTFOUND;
}

int ate_error_wrong_type_var(SHELL_VAR *var, const char *type)
{
   fprintf(stderr,
           "Variable '%s' should be type %s.\n",
           var->name,
           type);
   return EX_USAGE;
}

int ate_error_missing_usage(const char *what)
{
   fprintf(stderr, "Missing %s.\n", what);
   return EX_USAGE;
}

int ate_error_failed_to_create(const char *what)
{
   fprintf(stderr, "Failed to create %s\n", what);
   return EXECUTION_FAILURE;
}

int ate_error_corrupt_table(void)
{
   fprintf(stderr, "Corrupted array table.\n");
   return EX_BADASSIGN;
}

int ate_error_invalid_row_size(int requested)
{
   fprintf(stderr, "Invalid row size of %d.\n", requested);
   return EX_USAGE;
}

int ate_error_record_out_of_range(int requested, int limit)
{
   fprintf(stderr,
           "Mistakenly requested row %d from %d records.\n",
           requested,
           limit);

   return EX_NOTFOUND;
}

int ate_error_unexpected(void)
{
   fprintf(stderr, "Unexpected error.\n");
   return EXECUTION_FAILURE;
}
