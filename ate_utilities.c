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
      *result = (int)val;
      return True;
   }

   return False;
}

/**
 * @brief Safely convert a string to an integer
 * @param "result" [out]  pointer to where result should be set
 * @param "str"    [in]   character string with possible numeric value
 * @return True (!0) if successfully converted the string, False (0) if
 *                   the string is not a number
 */
bool get_long_from_string(long *result, const char *str)
{
   char *end_str;
   long val = strtol(str, &end_str, 10);
   if (end_str > str)
   {
      *result = val;
      return True;
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
 * @brief Replace SHELL_VAR value with string representing a long integer value.
 * @param "result"   [in,out]  variable whose value is to be changed
 * @param "value"    [in]      number to use for new value
 * @return EXECUTION_SUCCESS if successful
 */
int set_var_from_long(SHELL_VAR *result, long value)
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
 * @brief Converts arguments to integers and then compares as integers.
 *
 * This function returns `0` if either the @ref left or @ref right
 * argument cannot be converted to an integer.
 *
 * This function is used by @ref pwla_make_key and @ref pwla_seek_key
 * to enable sorting rows by integer key values.
 *
 * @param "left"   left-side comparator
 * @param "right"  right-side comparator
 * @return <0 if left < right, >0 if left > right
 *         0 ifleft==right OR if either @ref left or @ref right
 *         cannot be converted to an integer.
 */
int int_strcmp(const char *left, const char *right)
{
   int ileft=0, iright=0;
   if (get_int_from_string(&ileft, left))
      if (get_int_from_string(&iright, right))
         return ileft - iright;

   return 0;
}


/**
 * @brief Converts arguments to integers and then compares as integers.
 *
 * This function returns `0` if either the @ref left or @ref right
 * argument cannot be converted to a long integer.
 *
 * This function is used by @ref pwla_make_key and @ref pwla_seek_key
 * to enable sorting rows by integer key values.
 *
 * @param "left"   left-side comparator
 * @param "right"  right-side comparator
 * @return <0 if left < right, >0 if left > right
 *         0 ifleft==right OR if either @ref left or @ref right
 *         cannot be converted to an integer.
 */
int long_strcmp(const char *left, const char *right)
{
   long ileft=0, iright=0;
   if (get_long_from_string(&ileft, left))
      if (get_long_from_string(&iright, right))
      {
         // The difference between ileft and iright might
         // overflow an *int* return value, so I figure
         // two comparisons is computationally cheaper than
         // subtraction and division to fit a long into and int.
         // comparing
         if (ileft > iright)
            return 1;
         else if (ileft < iright)
            return -1;
         else
            return 0;
      }

   return 0;
}


int get_var_by_name(SHELL_VAR **var, const char *name, const char *action)
{
   SHELL_VAR *sv = NULL;
   if (!name || !(sv = find_variable(name)))
   {
      ate_register_missing_argument(name, action);
      return EX_USAGE;
   }

   *var = sv;
   return EXECUTION_SUCCESS;
}

/**
 * @brief Dispose of item referenced by SHELL_VAR::value.
 * @param "var"   variable whose contents are to be disposed of.
 *
 * Does the job of Bash's unavailable internal function,
 * dispose_variable_value.  Frees the memory pointed to by value
 * member according to its type.  The `value` member of the
 * SHELL_VAR will be NULL when done.
 */
void ate_dispose_variable_value(SHELL_VAR *var)
{
   if (array_p(var))
      array_dispose(array_cell(var));
   else if (assoc_p(var))
      assoc_dispose(assoc_cell(var));
   else if (function_p(var))
      dispose_command(function_cell(var));
   else if (nameref_p(var))
      FREE(nameref_cell(var));
   else
      FREE(value_cell(var));

   var->value = NULL;
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
   printf("Array lastref is %ld (%s).\n",
          array->lastref->ind,
          array->lastref->value);

   printf("Array max_index, num_elements are %ld, %ld.\n",
          (long int)array->max_index,
          (long int)array->num_elements);
   
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
   int retval = EX_USAGE;

   ARRAY *array = array_cell(head->array);

   // Confirm we're working with an indexed table
   if (head->row_count == 0)
   {
      ate_register_error("unable to reindex an unindexed table");
      goto early_exit;
   }

   // Make sure there are elements to index before proceeding:
   if (array->num_elements < head->row_size)
   {
      ate_register_error("not enough elements (%d) to make a complete row (%d fields)",
                         array->num_elements, head->row_size);
      goto early_exit;
   }

   retval = EXECUTION_SUCCESS;

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

  early_exit:
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

/**
 * @brief Change a table's row size and add empty fields to the end
 *        of each row.
 * @param "head"       [in] pointer to the AHEAD struct of an
 *                          initialized handle.
 * @param "new_columns [in] the number of fields to add to the row
 *                          size
 * @param "fill_value" [in] (optional) value to use for new array elements
 *
 * @return EXECUTION_SUCCESS if it works, otherwise EX_USAGE or EXECUTION_FAILURE
 */
int table_extend_rows(AHEAD *head, int new_columns, const char *fill_value)
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

   if (fill_value == NULL)
      fill_value = "";

   ARRAY_ELEMENT *field = NULL, *after_row = NULL;

   ARRAY_ELEMENT **row = row_start;
   while (row < end_index)
   {
      field = get_end_of_row(*row, row_size);
      after_row = field->next;

      el_count = 0;
      while (el_count < new_columns)
      {
         new_element = array_create_element(0, (char*)fill_value);

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

   if (field)
   {
      field->next = array->head;
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

  // early_exit:
   return retval;
}

/**
 * @brief Change a table's row size, removing out-of-range fields
 *        from the rows.
 * @param "head"             [in] pointer to the AHEAD struct of an
 *                                initialized handle.
 * @param "fields_to_remove" [in] the number of fields to remove from the
 *                                row size
 *
 * @return EXECUTION_SUCCESS if it works, otherwise EX_USAGE or EXECUTION_FAILURE
 */
int table_contract_rows(AHEAD *head, int fields_to_remove)
{
   int retval = EX_USAGE;

   ARRAY *array = array_cell(head->array);

   // Test for dangerous errors
   // Assert because this should be verified by pwla_resize_rows():
   assert(fields_to_remove > 0);

   if (fields_to_remove >= head->row_size)
   {
      ate_register_error("cannot remove more fields (%d) than exist (%d)",
                         fields_to_remove,
                         head->row_size);
      goto early_exit;
   }

   retval = EXECUTION_SUCCESS;

   // Only continue if there are enough elements to make a row
   // with the new row size:
   if (array->num_elements < head->row_size - fields_to_remove)
   {
      head->row_size -= fields_to_remove;
      goto early_exit;
   }

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
      while (el_count < fields_to_remove)
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
   head->row_size -= fields_to_remove;

  early_exit:
   return retval;
}

int update_row_array(SHELL_VAR *target_var, ARRAY_ELEMENT *source_row, int row_size)
{
   ARRAY *array = array_cell(target_var);
   array_flush(array);

   ARRAY_ELEMENT *ael = source_row;

   int index = 0;
   while (ael && index < row_size)
   {
      array_insert(array, index++, ael->value);
      ael = ael->next;
   }

   if (index < row_size)
   {
      ate_register_error("corrupted table: incomplete row");
      return EX_USAGE;
   }

   return EXECUTION_SUCCESS;
}

/**
 * @brief Invoke a shell function with the parameters of this function
 *        call
 * @param "function"  [in] validated shell function variable
 * @param  ...        [in] following arguments will be passed on as
 *                         parameters to the function.
 * @return EXECUTION_SUCCESS or a non-zero error value if it fails.
 */
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

/**
 * @brief Invoke a shell function with a word list
 * @param "function"  [in] validated shell function variable
 * @param "wl"        [in] a word list of arguments to submit
 *                         to the shell function.
 * @return EXECUTION_SUCCESS or a non-zero error value if it fails.
 */
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


/************************************
 * ARG_VARS group functions
 ***********************************/

/**
 * @defgroup CALLBACK_ARG_VARS SHELL_VARS to pass to callback functions.
 *
 * These variables are created to be accessed by name (as a nameref
 * variable) in callback functions as the means by which data is
 * passed to those functions.
 *
 * The variables are intended to be conceptually anonymous.
 *
 * @{
 */

/**
 * @brief Use a name stem to create a generic SHELL_VAR.
 * @param "var"    [out]  place to return new SHELL_VAR
 * @param "stem"   [in]   string on which to base the new name
 * @param "action" [in]   action name for reporting any errors
 *
 * @return EXECUTION_SUCCESS if the variable was created, EXECUTION_FAILURE if not.
 */
int create_var_by_stem(SHELL_VAR **var, const char *stem, const char *action)
{
   assert(stem);

   int len = strlen(stem) + 5;
   char *buffer = (char*)alloca(len);
   make_unique_name(buffer, len, stem);
   SHELL_VAR *sv = bind_variable(buffer, "", 0);
   if (sv)
   {
      *var = sv;
      return EXECUTION_SUCCESS;
   }

   ate_register_error("failed to create variable in action '%s'", action);
   return EXECUTION_FAILURE;
}


/**
 * @brief Use a name stem to create an array SHELL_VAR.
 * @param "var"    [out]  place to return new array SHELL_VAR
 * @param "stem"   [in]   string on which to base the new name
 * @param "action" [in]   action name for reporting any errors
 *
 * @return EXECUTION_SUCCESS if the variable was created, EXECUTION_FAILURE if not.
 */
int create_array_var_by_stem(SHELL_VAR **var, const char *stem, const char *action)
{
   assert(stem);

   int len = strlen(stem) + 5;
   char *buffer = (char*)alloca(len);
   make_unique_name(buffer, len, stem);

   // 2024-05-20: Don't attempt to make a local variable or it won't
   //             work for walk_rows callback function (as far as my
   //             limited knowledge understands).
   SHELL_VAR *sv = NULL;
   sv = make_new_array_variable(buffer);

   if (sv)
   {
      *var = sv;
      return EXECUTION_SUCCESS;
   }

   ate_register_error("failed to create array in action '%s'", action);
   return EXECUTION_FAILURE;
}

/**
 *  @}
 * <!-- terminate CALLBACK_ARG_VARS group -->
*/

/**
 * @defgroup INPUT_ARG_VARS Get User-requested SHELL_VARs
 *
 * For shell resources named on the command line that are required
 * for an action, these following functions handle return a SHELL_VAR
 * if the named resource exists, or registers an error for any
 * that prevents getting the resource (no name, missing variable).
 *
 * These functions will not return a SHELL_VAR unless it already
 * exists.
 *
 * @{
 */

/**
 * @brief Secure an `input` variable by name.
 *
 * This function is used to find a variable from which data will
 * be read.  As such, if it will return an error value if it cannot
 * be found.
 *
 * @param "rvar"  [out]  place to return the variable, if found
 * @param "name"  [in]   name of sought variable
 * @param "action" [in]  name of action for registering any errors
 * @return EXECUTION_SUCCESS if the variable was found and returned,
 *         EXECUTION_FAILURE if not.
 */
int get_handle_var_by_name_or_fail(SHELL_VAR **rvar,
                                   const char *name,
                                   const char *action)
{
   int retval = EX_USAGE;
   SHELL_VAR *sv = NULL;
   if (name == NULL)
      ate_register_error("failed to specific a handle name in '%s'", action);
   else
   {
      if ((sv = find_variable(name)))
      {
         if (ahead_p(sv))
         {
            AHEAD *ahead = ahead_cell(sv);
            retval = ate_check_head_integrity(ahead);
            if (retval == EXECUTION_SUCCESS)
               *rvar = sv;
         }
         else
            ate_register_error("variable '%s' is not a handle in action '%s'", name, action);
      }
      else
         ate_register_error("failed to find handle variable '%s' in action '%s'", name, action);
   }

   return retval;
}

/**
 * @brief Secure an `output` handle variable by a given name.
 *
 * This function will fail if the requested name is already in use.
 *
 * @param "rvar"   [out]  place to return the new handle, if successful
 * @param "name"   [in]   name to use for new handle
 * @param "ahead"  [in]   initialized AHEAD struct to attach to the handle
 * @param "action" [in]   action name for error messaging
 * @return EXECUTION_SUCCESS if successful, an error value if not.
 */
int create_handle_by_name_or_fail(SHELL_VAR **rvar,
                                  const char *name,
                                  AHEAD *ahead,
                                  const char *action)
{
   int retval = EX_USAGE;
   SHELL_VAR *sv = NULL;
   if (name == NULL)
      ate_register_error("failed to specific a handle name in '%s'", action);
   else if (ahead == NULL)
   {
      ate_register_error("internal error, failed to provide AHEAD for handle in '%s'", action);
      retval = EXECUTION_FAILURE;
   }
   else if (find_variable(name))
   {
      ate_register_error("action '%s' can't make variable '%s', it already exists", action, name);
      retval = EX_USAGE;
   }
   else
   {
      sv = bind_variable(name, "", 0);
      if (sv)
      {
         sv->value = (char*)ahead;
         VSETATTR(sv, att_special);
         *rvar = sv;
         retval = EXECUTION_SUCCESS;
      }
      else
      {
         ate_register_error("failed to bind a variable named '%s' in '%s'", name, action);
         retval = EXECUTION_FAILURE;
      }
   }

   return retval;
}

/**
 * @brief Secure an `input` array variable by name, generate error on failure
 * @param "rvar"   [out]  place to return the variable if found
 * @param "name"   [in]   name of variable to seek
 * @param "action" [in]   action name for error messaging
 * @return EXECUTION_SUCCESS if variable was found, a non-zero error
 *         code if it failed
 */
int get_array_var_by_name_or_fail(SHELL_VAR **rvar,
                                  const char *name,
                                  const char *action)
{
   int retval = EX_USAGE;
   if (name == NULL)
      ate_register_error("failed to specific a array name in '%s'", action);
   else
   {
      SHELL_VAR *sv = find_variable(name);
      if (sv)
      {
         if (array_p(sv))
         {
            *rvar = sv;
            retval = EXECUTION_SUCCESS;
         }
         else
            ate_register_error("variable '%s' must be an array in '%s'", name, action);
      }
      else
         ate_register_error("failed to find array '%s' in action '%s'", name, action);
   }

   return retval;
}

/**
 * @brief Secure an `input` function variable by name, register the
 *        error if it doesn't exist.
 * @param "rvar"   [out]  place to return the function variable if found
 * @param "name"   [in]   name of variable to seek
 * @param "action" [in]   action name for error messaging
 * @return EXECUTION_SUCCESS if the function was found, a non-zero error
 *         code if it failed
 */
int get_function_by_name_or_fail(SHELL_VAR **rvar,
                                 const char *name,
                                 const char *action)
{
   int retval = EX_USAGE;
   if (name == NULL)
      ate_register_error("failed to specific a callback function name in '%s'", action);
   else
   {
      SHELL_VAR *sv = find_function(name);
      if (sv)
      {
         if (function_p(sv))
         {
            *rvar = sv;
            retval = EXECUTION_SUCCESS;
         }
         else
            ate_register_error("'%s' must be a shell function name in '%s'", name, action);
      }
      else
         ate_register_error("failed to find shell function '%s' in action '%s'", name, action);
   }

   return retval;
}

/**
 * @}
 * <!-- terminate INPUT_ARG_VARS group -->
 */

/**
 * @defgroup RESULT_ARG_VARS Secure Result SHELL_VARs
 *
 * These functions handle replacing the default result value variables are
 * available when using `-v` or `-a` for value and array result variables.
 * @{
 */

/**
 * @brief Secure the `output` value SHELL_VAR by a given or the default name.
 *
 * Used by actions like `get_row_size`, `get_row_count`, `get_array_name`,
 * and 'seek_key'.  this function will create or commandeer a variable,
 * using either the given name or a default name if no name is provided.
 *
 * @param "rvar"         [out] place to return the SHELL_VAR
 * @param "name"         [in]  name to use for the SHELL_VAR (may be null)
 * @param "default_name" [in]  name to use if @p name not used
 * @param "action"       [in]  action name to use for error messaging
 * @return EXECUTION_SUCCESS if the variable was secured, otherwise a
 *         non-zero error code
 */
int create_var_by_given_or_default_name(SHELL_VAR **rvar,
                                        const char *name,
                                        const char *default_name,
                                        const char *action)
{
   int retval = EX_USAGE;
   SHELL_VAR *sv = NULL;
   if (name == NULL)
      name = default_name;

   if (name == NULL)
   {
      ate_register_error("internal failure to ensure variable name in '%s'", action);
      retval = EXECUTION_FAILURE;
   }
   else
   {
      sv = find_variable(name);
      if (sv)
      {
         if (sv->attributes & (att_assoc || att_array))
         {
            ate_register_error("attempting to change array variable %s to a non-array in %s",
                               name, action);
            retval = EX_USAGE;
            goto early_exit;
         }
         else
         {
            ate_dispose_variable_value(sv);
            sv->attributes &= ~att_invisible;
         }
      }
      else
         sv = bind_variable(name, "", 0);

      if (sv)
      {
         *rvar = sv;
         retval = EXECUTION_SUCCESS;
      }
      else
      {
         ate_register_error("internal failure to bind variable '%s' in '%'", name, action);
         retval = EXECUTION_SUCCESS;
      }
   }

  early_exit:
   return retval;
}

/**
 * @brief Secure the `output` array SHELL_VAR by a given or the default name
 *
 * Used by actions like `get_row` and `get_field_sizes`, this function
 * will create or commandeer an array variable, using either the given
 * name or a default name if no name is provided.
 *
 * @param "rvar"         [out] place to return the array SHELL_VAR
 * @param "name"         [in]  name to use for the array SHELL_VAR (may be null)
 * @param "default_name" [in]  name to use if @p name not used
 * @param "action"       [in]  action name to use for error messaging
 * @return EXECUTION_SUCCESS if the array variable was secured,
 *         otherwise a non-zero error code
 */
int create_array_var_by_given_or_default_name(SHELL_VAR **rvar,
                                              const char *name,
                                              const char *default_name,
                                              const char *action)
{
   int retval = EX_USAGE;
   SHELL_VAR *sv = NULL;
   if (name == NULL)
      name = default_name;

   if (name == NULL)
   {
      ate_register_error("internal failure to ensure variable name in '%s'", action);
      retval = EXECUTION_FAILURE;
   }
   else
   {
      sv = find_variable(name);
      if (sv)
      {
         if (!(sv->attributes & att_array))
         {
            ate_register_error("attempting to use non-array variable %s as an array in %s",
                               name, action);
            retval = EX_USAGE;
            goto early_exit;
         }
         else
         {
            ate_dispose_variable_value(sv);
            sv->attributes &= ~att_invisible;
            sv->value = (char*)array_create();
         }
      }
      else
         sv = make_new_array_variable((char*)name);

      if (sv)
      {
         *rvar = sv;
         retval = EXECUTION_SUCCESS;
      }
      else
      {
         ate_register_error("internal failure to bind array variable '%s' in '%'", name, action);
         retval = EXECUTION_SUCCESS;
      }
   }

  early_exit:
   return retval;
}

/**
 * @}
 * <!-- terminate RESULT_ARG_VARS group -->
 */

/**
 * @brief Create a special (handle) SHELL_VAR to be initialized later
 *        by an action.
 * @param "rvar"         [out] place to return the special SHELL_VAR
 * @param "name"         [in]  name to use for the special SHELL_VAR (may be null)
 * @param "action"       [in]  action name to use for error messaging
 * @return EXECUTION_SUCCESS if the special variable was secured,
 *         otherwise a non-zero error code
 */
int create_special_var_by_name(SHELL_VAR **rvar, const char *name, const char *action)
{
   int retval = EX_USAGE;

   if (name == NULL)
   {
      ate_register_missing_argument("new_handle_name", action);
      goto early_exit;
   }

   if (find_variable(name))
      unbind_variable(name);

   SHELL_VAR *var = bind_variable(name, NULL, 0);
   if (var)
   {
      VSETATTR(var, att_special);
      *rvar = var;
      retval = EXECUTION_SUCCESS;
   }

  early_exit:
   return retval;
}
