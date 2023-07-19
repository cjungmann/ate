/**
 * @file ate_utilities.c
 * @brief A collection of utilities not directly involved with specific
 *        actions.
 */

#include "ate_utilities.h"
#include "ate_errors.h"
#include "word_list_stack.h"

// Copied from bash source header execute_cmd.h:
extern int execute_command PARAMS((COMMAND *));

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

/**
 * @brief Use name stem to find an unused SHELL_VAR name.
 * @param "buffer"   [in,out]  memory to which the name will be written
 * @param "bufflen"  [in]      size of the @p buffer
 * @param "stem"     [in]      stem to use for new name
 *
 * @return True if successful finding a unique name
 */
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
 * @brief Safely convert a string to an integer
 * @param "result" [out]  pointer to where result should be set
 * @param "str"    [in]   character string with possible numeric value
 * @return True (!0) if successfully converted the string, False (0) if
 *                   the string is not a number
 */
bool get_int_from_string(int *result, const char *str)
{
   char *end_str;
   long val = strtol(str, &end_str, 10);
   if (end_str > str)
   {
      *result = (long)val;
      return True;
   }

   return False;
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

/**
 * @brief Replace SHELL_VAR value with string representing an integer value.
 * @param "result"   [in,out]  variable whose value is to be changed
 * @param "value"    [in]      number to use for new value
 * @return EXECUTION_SUCCESS if successful
 */
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
 * @brief Make or reuse a SHELL_VAR of the specified name.
 *
 * First search for an existing variable of the specified @p name,
 * create one if not found, then clean out whichever variable and set
 * the attributes.
 *
 * @param "name"       [in]  name of requested SHELL_VAR
 * @param "attributes" [in]  attributes to be set on the SHELL_VAR
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
 * @brief Create or reuse a SHELL_VAR as an ARRAY.
 * @param "var"  [out]  where the new/reused SHELL_VAR is returned
 * @param "name" [in]   name of requested variable
 * @return True if successful
 */
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
 * @param "head"        [out]  where value will be returned, if successful
 * @param "name_handle" [in]   name of AHEAD handle being sought
 * @return EXECUTION_SUCCESS if found and validated, appropriate
 *         error value if not.
 */
int get_handle_from_name(AHEAD **head, const char *name_handle, const char *action_name)
{
   if (name_handle == NULL)
   {
      ate_register_missing_argument("ate handle name", action_name);
      return EX_USAGE;
   }

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
         ate_register_error("variable '%s' is not an ate handle", name_handle);
         return EX_USAGE;
      }
   }

   ate_register_error("ate handle variable '%s' was not found", name_handle);
   return EX_USAGE;
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

      ate_register_variable_wrong_type(name, get_type_name_from_attribute(attributes));
      return EX_USAGE;
   }

   ate_register_variable_not_found(name);
   return EX_USAGE;
}

/**
 * @brief associate a name with an ARRAY_ELEMENT test if it's the ARRAY head element
 * Implemented as MACRO to ensure inline execution
 * @param "el"   Element to test if it's the array head
 */
#define array_element_is_head(el) ((el)->ind == -1)

/**
 * @brief Returns an array with a copy of a series of elements
 *        from a source array.
 *
 * Used by @ref ate_action_sort and @ref ate_action_get_row
 *
 * @param "new_array"        [out]  Where the new or reused array
 *                                  variable will be returned
 * @param "starting_element" [in]   the first of a series of array
 *                                  elements that will be copied to
 *                                  the target array
 * @param "el_count"         [in]   number of elements to copy
 * @param "name"             [in]   name of variable to create or
 *                                  reuse
 *
 * @return EXECUTION_SUCCESS or error code
 */
int clone_range_to_array(SHELL_VAR **new_array,
                         ARRAY_ELEMENT *starting_element,
                         int el_count,
                         const char *name)
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
   ARRAY_ELEMENT *source_ptr = starting_element;
   for (;
        index < el_count && ! array_element_is_head(source_ptr);
        ++index, source_ptr = source_ptr->next
        )
      array_insert(target_array, index, savestring(source_ptr->value));

   if (index < el_count)
   {
      ate_register_corrupt_table();
      retval = EX_USAGE;
   }
   else
      *new_array = var;

   return retval;
}

/**
 * @brief Print array details including elements and their indicies.
 *
 * Since the code is taking *major liberties* with the organization
 * of an array, it behooves me to prepare a test to show even
 * otherwise hidden details about an array to confirm the integrity
 * of the modified array.
 *
 * @param "array"  array to display
 */
void survey_array(ARRAY *array)
{
   // Test array
   printf("\n\nTest the array:\n");
   ARRAY_ELEMENT *ptr = array->head->next;
   while (ptr != array->head)
   {
      printf("%2ld: '%s'\n", ptr->ind, ptr->value);
      ptr = ptr->next;
   }
   printf("Array lastref is %ld (%s).\n", array->lastref->ind, array->lastref->value);
   printf("Array max_index, num_elements are %ld, %d.\n", array->max_index, array->num_elements);
   printf("How does it look?\n");
}

/**
 * @brief Reorganize array elements to match order of indexed rows.
 *
 * This is a pretty presumptuous function, changing element pointers
 * at the beginning and end of rows to make a new linked order.
 *
 * Calling functions are responsible for ensuring that the array
 * is in an appropriate state to run this function.  That means
 * there should be no orphan elements (not reachable through row
 * indexes, within the scope of a row).
 *
 * @param "head"   Initialzed head with an installed array and
 *                 indexed rows.
 * @return EXECUTION_SUCCESS for all, until we find an error
 *         condition for which we should test and abort.
 */
int reindex_array_elements(AHEAD *head)
{
   int retval = EXECUTION_SUCCESS;

   ARRAY *array = array_cell(head->array);
   ARRAY_ELEMENT **row = head->rows;
   ARRAY_ELEMENT **end_index = row + head->row_count;

   int new_index=0;
   ARRAY_ELEMENT *end_of_last_row = NULL;

   while (row < end_index)
   {
      ARRAY_ELEMENT *ptr = *row;
      if (!ptr)
      {
         fprintf(stderr, "Big mistake! lost the thread of the array.");
         break;
      }

      // Adjust connections
      if (end_of_last_row)
      {
         ptr->prev = end_of_last_row;
         end_of_last_row->next = ptr;
      }
      else
      {
         ptr->prev = array->head;
         array->head->next = ptr;
      }

      // Set sequential element indicies
      while (ptr)
      {
         ptr->ind = new_index++;
         if (0 == (new_index % head->row_size))
         {
            end_of_last_row = ptr;
            break;
         }
         else
            ptr = ptr->next;
      }

      ++row;
   }

   // Complete the circle
   end_of_last_row->next = array->head;
   array->lastref = end_of_last_row;

   array->max_index = end_of_last_row->ind;

  // early_exit:
   return retval;
}

/**
 * @brief Returns last array element of a virtual row
 * @param "row"       head element of the row
 * @param "row_size"  from AHEAD struct to know where is the end
 * @return the last element of the specified row.
 */
ARRAY_ELEMENT *get_end_of_row(ARRAY_ELEMENT *row, int row_size)
{
   int count=0;

   // Precount because we want row[row_size-1].
   // row[row_size] is (conceptually) the first element of the next row.
   while (++count < row_size)
      row = row->next;

   return row;
}

int table_extend_rows(AHEAD *head, int new_columns)
{
   int retval = EXECUTION_SUCCESS;

   ARRAY *array = array_cell(head->array);

   // Row-looping variables
   ARRAY_ELEMENT **row_start = head->rows;
   ARRAY_ELEMENT **end_index = row_start + head->row_count;

   ARRAY_ELEMENT *new_element;
   int row_size = head->row_size;
   int el_count;
   int new_elements = 0;

   ARRAY_ELEMENT *field, *after_row;

   ARRAY_ELEMENT **row = row_start;
   while (row < end_index)
   {
      field = get_end_of_row(*row, row_size);
      after_row = field->next;

      el_count = 0;
      while (el_count < new_columns)
      {
         new_element = array_create_element(0, "");

         new_element->prev = field;
         field->next = new_element;

         field = new_element;

         ++el_count;
         ++new_elements;
      }

      assert(field->next == NULL);
      field->next = after_row;
      after_row->prev = field;

      ++row;
   }

   // Assuming *after_row, which is what the ::next member of the
   // last element of the last row should be pointing to, is the
   // ::head member of the ARRAY:
   assert(after_row == array->head);

   // The last field added should be the last new element of the
   // last row of the table, and thus that last element of the
   // array.  It should be pointing to the array head
   assert(field->next == array->head);

   if (field)
   {
      array->lastref = field;

      // This might be dangerous: I'm assuming that
      // all the additions were OK, but we also don't
      // want to leave an incorrect value, which is
      // tested elsewhere.
      array->num_elements += new_elements;
   }

   // Update first because reindex_array_elements uses
   // head->row_size to process the elements.
   head->row_size += new_columns;

   // Call to assign appropriate indexes, or future
   // additions will fail:
   retval = reindex_array_elements(head);

  // early_exit:
   return retval;
}

int table_contract_rows(AHEAD *head, int columns_to_remove)
{
   int retval = EXECUTION_SUCCESS;

   ARRAY *array = array_cell(head->array);

   // Row-looping variables
   ARRAY_ELEMENT **row_start = head->rows;
   ARRAY_ELEMENT **end_index = row_start + head->row_count;

   ARRAY_ELEMENT *prev_element;
   int row_size = head->row_size;
   int el_count;
   int removed_elements = 0;

   ARRAY_ELEMENT *field, *after_row;

   ARRAY_ELEMENT **row = row_start;
   while (row < end_index)
   {
      field = get_end_of_row(*row, row_size);
      after_row = field->next;

      el_count = 0;
      while (el_count < columns_to_remove)
      {
         prev_element = field->prev;
         array_dispose_element(field);
         field = prev_element;

         ++el_count;
         ++removed_elements;
      }

      field->next = after_row;
      after_row->prev = field;

      ++row;
   }

   // Assuming *after_row, which is what the ::next member of the
   // last element of the last row should be pointing to, is the
   // ::head member of the ARRAY:
   assert(after_row == array->head);

   // The last field added should be the last new element of the
   // last row of the table, and thus that last element of the
   // array.  It should be pointing to the array head
   assert(field->next == array->head);

   // Update array members with new dimensions
   if (field)
   {
      array->lastref = field;

      // This might be dangerous: I'm assuming that
      // all the deletions were OK, but we also don't
      // want to leave an incorrect value, which is
      // tested elsewhere.
      array->num_elements -= removed_elements;
   }

   // Update first because reindex_array_elements uses
   // head->row_size to process the elements.
   head->row_size -= columns_to_remove;

   // Call to assign appropriate indexes, or future
   // additions will fail:
   retval = reindex_array_elements(head);

  // early_exit:
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

int invoke_shell_function_word_list(SHELL_VAR *function, WORD_LIST *wl)
{
   WORD_LIST *params = NULL;
   WL_APPEND(params, function->name);
   params->next = wl;

   int cmd_flags = CMD_INHIBIT_EXPANSION | CMD_STDPATH;

   COMMAND *command;
   command = make_bare_simple_command();
   command->value.Simple->words = params;
   command->value.Simple->redirects = (REDIRECT*)NULL;
   command->flags = command->value.Simple->flags = cmd_flags;

   return execute_command(command);
}
