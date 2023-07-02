#include "ate_utilities.h"
#include "ate_errors.h"
#include "word_list_stack.h"

// Copied from bash source header execute_cmd.h:
extern int execute_command PARAMS((COMMAND *));

#include <stdio.h>
#include <stdarg.h>


bool make_unique_name(char *buffer, int bufflen, const char *stem)
{
   int len = strlen(stem);
   if (bufflen < len + 3)
      return 0;

   int numlen = bufflen - len - 1;

   char **vars = all_variables_matching_prefix(stem);
   char **ptr = vars;
   int cur_val,  max_val = 0;
   while (*ptr)
   {
      const char *str = *ptr;
      cur_val = atoi(&str[len]);
      if (cur_val > max_val)
         max_val = cur_val;

      ++ptr;
   }

   snprintf(buffer, bufflen, "%s%0*d", stem, numlen, max_val+1);
   return 1;
}


/**
 * @brief Get string value at specified index of the WORD_LIST.
 * @param "result"  [out]  where value will be stored, if found
 * @param "list"    [in]   list to search
 * @param "index"   [in]   0-based index whose value to return
 *
 * @return True if found, False if index beyond bounds of WORD_LIST.
 */
bool get_string_from_list(const char **result, WORD_LIST *list, int index)
{
   int counter = 0;
   while (list)
   {
      if (counter++ == index)
      {
         *result = list->word->word;
         return True;
      }

      list = list->next;
   }

   return False;
}

/**
 * @brief Get integer value at specified index of the WORD_LIST.
 * @param "result"  [out]  where value will be stored, if found
 * @param "list"    [in]   list to search
 * @param "index"   [in]   0-based index whose value to return
 *
 * @return True if found, False if index beyond bounds of WORD_LIST.
 */
bool get_int_from_list(int *result, WORD_LIST *list, int index)
{
   const char *str;
   if (get_string_from_list(&str, list, index))
   {
      *result = atoi(str);
      return True;
   }

   return False;
}

/**
 * @brief Get new or existing SHELL_VAR* with name at the specified
 *        index of the WORD_LIST.
 * @param "result"  [out]  where value will be stored, if found
 * @param "list"    [in]   list to search
 * @param "index"   [in]   0-based index whose value to return
 *
 * @return True if found, False if index beyond bounds of WORD_LIST.
 */
bool get_var_from_list(SHELL_VAR **result, WORD_LIST *list, int index)
{
   SHELL_VAR *var;
   const char *name;
   if (get_string_from_list(&name, list, index))
   {
      var = find_variable(name);
      if (!var)
         var = bind_variable(name, "", 0);

      if (var)
      {
         *result = var;
         return True;
      }
   }

   return False;
}

int set_var_from_int(SHELL_VAR *result, int value)
{
   char *intstr = itos(value);
   if (intstr && intstr[0])
   {
      ate_dispose_variable_value(result);
      result->value = intstr;
      return EXECUTION_SUCCESS;
   }

   return EX_BADUSAGE;
}

/**
 * @brief Dispose of item referenced by SHELL_VAR::value.
 * @param "var"   variable whose contents are to be disposed of.
 *
 * Does the job of Bash's unavailable internal function,
 * dispose_variable_value.  Frees the memory pointed to by value
 * member according to its type.
 */
void ate_dispose_variable_value(SHELL_VAR *var)
{
   if (array_p(var))
      var->value = NULL;
   else if (assoc_p(var))
      var->value = NULL;
   else if (function_p(var))
      dispose_command(function_cell(var));
   else if (nameref_p(var))
      FREE(nameref_cell(var));
   else
      FREE(value_cell(var));

   var->value = NULL;
}

/**
 * @brief Find and dipose value of existing variable, or make new one
 *
 * First search for an existing variable of the specified @p name,
 * create one if not found, then clean out whichever variable and set
 * the attributes.
 *
 * @param "var"  [out]   Pointer to which the SHELL_VAR will be returned
 * @param "name" [in]    Name to search or to use when creating a variable
 * @return Prepared variable
 */
SHELL_VAR *ate_get_prepared_variable(const char *name, int attributes)
{
   SHELL_VAR *svar = find_variable(name);
   if (!svar)
      svar = bind_variable(name, "", 0);

   ate_dispose_variable_value(svar);
   svar->attributes = attributes;

   return svar;
}

bool prepare_clean_array_var(SHELL_VAR **var, const char *name)
{
   SHELL_VAR *tvar;
   tvar = find_variable(name);
   if (tvar)
      dispose_variable(tvar);

   tvar = make_new_array_variable((char*)name);
   if (tvar)
   {
      *var = tvar;
      return True;
   }

   return False;
}

/**
 * @brief Returns validate AHEAD handle if possible.
 *
 * This function will print error messages to stderr.
 *
 * @param "head" [out]  where value will be returned, if valid
 * @param "name" [in]   name of AHEAD handle being sought
 * @return EXECUTION_SUCCESS if found and validated, appropriate
 *         error value if not.
 */
int get_handle_from_name(AHEAD **head, const char *name_handle)
{
   SHELL_VAR *svar = find_variable(name_handle);
   if (svar)
   {
      if (ahead_p(svar))
      {
         *head = ahead_cell(svar);
         return EXECUTION_SUCCESS;
      }
      else
         return ate_error_wrong_type_var(svar, "ate handle");
   }

   return ate_error_var_not_found(name_handle);
}

/**
 * @brief Returns variable type name for use in warning messages.
 * @param "attribute"  filter attribute, only considers att_array, att_function,
 *                     att_integer, and att_assoc.
 * @returns An identifying string
 */
const char *get_type_name_from_attribute(int attribute)
{
   if (attribute & att_array)
      return "ARRAY";
   else if (attribute & att_integer)
      return "INTEGER";
   else if (attribute & att_function)
      return "FUNCTION";
   else if (attribute & att_assoc)
      return "ASSOCIATIVE ARRAY";
   else
      return "UNKNOWN TYPE";
}

/**
 * @brief Seeks and type-validates a SHELL_VAR by name
 *
 * This function will print error messages to stderr.
 *
 * @param "retval"     [out]  return of found and type-validated
 *                            SHELL_VAR
 * @param "name"       [in]   name of desired SHELL_VAR
 * @param "attributes" [in]   attribute to identify desired variable
 *                            type (att_integer, att_array,
 *                            att_function, or att_assoc).  Pass a '0'
 *                            if type agnostic.
 * @return EXECUTION_SUCCESS for success, otherwise an appropriate
 *         error value.
 */
int get_shell_var_by_name_and_type(SHELL_VAR **retval, const char *name, int attributes)
{
   SHELL_VAR *var = find_variable(name);
   if (var)
   {
      if (attributes==0 || var->attributes & attributes)
      {
         *retval = var;
         return EXECUTION_SUCCESS;
      }
      else
         return ate_error_wrong_type_var(var, get_type_name_from_attribute(attributes));
   }

   return ate_error_var_not_found(name);
}

int clone_range_to_array(SHELL_VAR **new_array, ARRAY *source_array, int el_count, const char *name)
{
   int retval = EXECUTION_SUCCESS;

   SHELL_VAR *var = find_variable(name);
   ARRAY *target_array = NULL;
   if (var)
   {
      ate_dispose_variable_value(var);
      target_array = array_create();
      var->value = (char*)target_array;
      var->attributes = att_array;
   }
   else
   {
      var = make_new_array_variable((char*)name);
      target_array = array_cell(var);
   }

   int index = 0;
   ARRAY_ELEMENT *source_head = array_head(source_array);
   ARRAY_ELEMENT *source_ptr = source_head->next;
   for (;
        index < el_count && source_ptr != source_head;
        ++index, source_ptr = source_ptr->next)
   {
      array_insert(target_array, index, savestring(source_ptr->value));
   }

   if (index < el_count)
      retval = ate_error_corrupt_table();
   else
      *new_array = var;

   return retval;
}

int invoke_shell_function(SHELL_VAR *function, ...)
{
   WORD_LIST *list = NULL, *tail = NULL;

   WL_APPEND(tail, function->name);
   list = tail;

   va_list args_list;
   va_start(args_list, function);
   const char *arg;
   while ((arg = va_arg(args_list, const char*)))
      WL_APPEND(tail, arg);

   va_end(args_list);

   int cmd_flags = CMD_INHIBIT_EXPANSION | CMD_STDPATH;

   COMMAND *command;
   command = make_bare_simple_command();
   command->value.Simple->words = list;
   command->value.Simple->redirects = (REDIRECT*)NULL;
   command->flags = command->value.Simple->flags = cmd_flags;

   return execute_command(command);
}
