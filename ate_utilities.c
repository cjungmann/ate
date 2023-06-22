#include "ate_utilities.h"

#include <stdio.h>

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
      {
         fprintf(stderr,
                 "Variable '%s' is not an appropriate handle.\n",
                 name_handle);

         return EX_BADUSAGE;
      }
   }

   fprintf(stderr, "Failed to find handle '%s'\n", name_handle);
   return EX_NOTFOUND;
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
      {
         fprintf(stderr,
                 "Variable '%s' is not a %s variable.\n",
                 name,
                 get_type_name_from_attribute(attributes));
         return EX_BADUSAGE;
      }
   }
   else
   {
      fprintf(stderr, "Failed to find variable '%s'.\n", name);
      return EX_NOTFOUND;
   }
}
