/**
 * @file ate_action_walk_rows.c
 * @brief Isolates walk_rows action function
 */

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "ate_action.h"
#include "ate_utilities.h"
#include "ate_errors.h"
#include "word_list_stack.h"

#define STEP_ARRAY_STEM "ATE_WALK_ROW_STEP_"

/**
 * @brief Initiates a series of invocations of a callback function
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         must include a callback function name,
 *                        optionally followed by -s starting_row
 *                        and/or -c count_of_rows to limit the output,
 *                        followed by optional additional string
 *                        values that will be passed to the callback
 *                        function.
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_walk_rows(const char *name_handle, const char *name_value,
                         const char *name_array, WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle, "walk_rows");
   if (retval)
      goto early_exit;

   // Create a new array shell_var to use for transferring rows
   static const char *stem = STEP_ARRAY_STEM;
   const static int buff_len = 4 + sizeof STEP_ARRAY_STEM;

   SHELL_VAR *array_var = NULL;
   ARRAY *array = NULL;
   char *arr_name = (char*)alloca(buff_len);
   if (arr_name && make_unique_name(arr_name, buff_len, stem))
   {
      array_var = make_new_array_variable(arr_name);
      if (array_var)
         array = array_cell(array_var);
   }

   if (NULL == array)
   {
      ate_register_error("unexpected failure to create an array");
      retval = EXECUTION_FAILURE;
      goto early_exit;
   }

   // No need to continue if there are no rows
   if (handle->row_count == 0)
   {
      ate_register_empty_table(name_handle);
      goto early_exit;
   }

   // Variables to be filled from the arguments
   const char *func_name = NULL;
   SHELL_VAR *func_var = NULL;
   int starting_index = 0;
   int rows_to_walk = handle->row_count;
   WORD_LIST *new_extra=NULL, *new_tail=NULL;

   WORD_LIST *ptr = extra;
   while (ptr)
   {
      const char *curarg = ptr->word->word;
      if (curarg[0] == '-')
      {
         int *iptr = NULL;
         if (curarg[1] == 's')
            iptr = &starting_index;
         else if (curarg[1] == 'c')
            iptr = &rows_to_walk;
         else
         {
            ate_register_error("invalid option '-%c'", curarg[1]);
            goto early_exit;
         }

         // Get pointer to option value (next letter or next argument)
         const char *val = &curarg[2];
         if (*val == '\0')
            val = ptr->next->word->word;

         if (val && get_int_from_string(iptr, val))
         {
            // Update int pointer to next needed value
            if (iptr == &starting_index)
               iptr = &rows_to_walk;
            else if (iptr == &rows_to_walk)
               iptr = NULL;

            // Skip if borrowed next argument
            if (val != &curarg[2])
               ptr = ptr->next;
         }
         else
         {
            ate_register_argument_wrong_type(val, "number");
            goto early_exit;
         }
      }
      else if (func_name == NULL)
      {
         // We process the name later
         func_name = ptr->word->word;
      }
      else
      {
         // Save everything not handled above, to send
         // to the callback function
         WL_APPEND(new_tail, ptr->word->word);
         if (new_extra == NULL)
            new_extra = new_tail;
      }

      ptr = ptr->next;
   }

   // Check the argument variables
   if (!func_name || NULL == (func_var = find_function(func_name)))
   {
      ate_register_error("missing or misspelled callback function name");
      goto early_exit;
   }

   // check range sanity
   if ((unsigned)starting_index >= handle->row_count || rows_to_walk < 1)
   {
      ate_register_error("invalid range or row count values");
      goto early_exit;
   }

   char row_index_string[20];
   WORD_LIST *params = NULL, *params_tail = NULL;
   WL_APPEND(params_tail, arr_name);
   params = params_tail;
   WL_APPEND(params_tail, "1");
   params_tail->word->word=row_index_string;
   WL_APPEND(params_tail, name_handle);
   params_tail->next = new_extra;

   ARRAY_ELEMENT *current_ael;

   // Uniquely process the callback exitcode,
   // 0 to continue,
   // 1 to stop
   int callback_result;

   int row_limit = rows_to_walk + starting_index;
   if (row_limit > handle->row_count)
      row_limit = handle->row_count;

   for (int i = starting_index; i < row_limit; ++i)
   {
      array_flush(array);
      if (NULL == (current_ael = ate_get_indexed_row(handle, i)))
      {
         ate_register_corrupt_table();
         retval = EXECUTION_FAILURE;
         goto early_exit;
      }

      for (int ei=0; ei < handle->row_size && current_ael; ++ei)
      {
         array_insert(array, ei, current_ael->value);
         current_ael = current_ael->next;
      }

      // Overwrite row-index string buffer with current value
      snprintf(row_index_string, sizeof(row_index_string), "%d", i);

      callback_result = invoke_shell_function_word_list(func_var, params);

      if (callback_result != 0)
         break;
   }

  early_exit:
   return retval;
}
