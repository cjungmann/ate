/**
 * @file ate_errors.c
 * @brief Single location for printing error messages.  Prepares development
 *        to disable messages in a single place.
 */

#include <builtins.h>
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif
#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

// Declared in ate.c
extern const char Error_Name[];
#include "ate_utilities.h"

void save_to_error_shell_var(const char *str)
{
   SHELL_VAR *var = find_variable(Error_Name);
   if (var == NULL)
      var = bind_variable(Error_Name, (char*)str, 0);
   else
   {
      ate_dispose_variable_value(var);
      var->value = savestring(str);
      var->attributes = 0;
   }
}

void ate_register_error(const char *format, ...)
{
   va_list args_list;

   int len = 0;
   char *buffer = NULL;

   while(1)
   {
      va_start(args_list, format);
      len = vsnprintf(buffer, len, format, args_list);
      va_end(args_list);

      if (buffer == NULL)
         buffer = alloca(++len);
      else
      {
         save_to_error_shell_var(buffer);
         break;
      }
   }
}


void ate_register_variable_not_found(const char *name)
{
   ate_register_error("variable '%s' is not found", name);
}

void ate_register_function_not_found(const char *name)
{
   ate_register_error("function '%s' is not found", name);
}

void ate_register_variable_wrong_type(const char *name, const char *desired_type)
{
   ate_register_error("variable '%s' should be type %s", name, desired_type);
}

void ate_register_argument_wrong_type(const char *value, const char *desired_type)
{
   ate_register_error("argument value '%s' should be type %s", value, desired_type);
}

void ate_register_empty_table(const char *handle_name)
{
   ate_register_error("ate handle '%s' is empty or unindexed", handle_name);
}

void ate_register_corrupt_table(void)
{
   ate_register_error("the ate handle is corrupted");
}

void ate_register_invalid_row_index(int requested, int available)
{
   ate_register_error("index %d is invalid in list of %d rows", requested, available);
}

void ate_register_invalid_row_size(int row_size, int el_count)
{
   ate_register_error("invalid row size: %d does not divide evenly into %d elements",
                      row_size, el_count);
}

void ate_register_missing_argument(const char *name, const char *action)
{
   ate_register_error("missing '%s' argument in action '%s'", name, action);
}

void ate_register_failed_to_create(const char *name)
{
   ate_register_error("failed to create variable '%s'", name);
}

void ate_register_unexpected_error(const char *doing)
{
   ate_register_error("encountered unexpected error while %s", doing);
}

