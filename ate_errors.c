#include <builtins.h>
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif
#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>

int ate_error_not_found(const char *what)
{
   fprintf(stderr, "Failed to find %s.\n", what);
   return EX_NOTFOUND;
}

int ate_error_var_not_found(const char *name)
{
   fprintf(stderr, "Failed to find variable '%s'.\n", name);
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
