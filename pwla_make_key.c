#include "pwla.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

int pwla_make_key_qsort_callback(const void *left, const void *right)
{
   ARRAY_ELEMENT *el_left = *(ARRAY_ELEMENT**)left;
   ARRAY_ELEMENT *el_right = *(ARRAY_ELEMENT**)right;
   return strcmp(el_left->value, el_right->value);
}

/**
 * @brief Make an index with which one can submit a name and get a
 *        row index in return.
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 *
 * see man ate(1)
 */
int pwla_make_key(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *function_name = NULL;
   const char *new_handle_name = NULL;

   ARG_TARGET walk_rows_targets[] = {
      { "handle_name",     AL_ARG, &handle_name},
      { "function_name",   AL_ARG, &function_name},
      { "new_handle_name", AL_ARG, &new_handle_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(walk_rows_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "make_key")))
      goto early_exit;

   AHEAD *ahead = ahead_cell(handle_var);

   SHELL_VAR *function_var = NULL;
   if ((retval = get_function_by_name_or_fail(&function_var,
                                              function_name,
                                              "make_key")))
      goto early_exit;

   SHELL_VAR *new_handle_var = NULL;
   if ((retval = create_special_var_by_name(&new_handle_var,
                                            new_handle_name,
                                            "make_key")))
      goto early_exit;

   // Prepare variables and a WORD_LIST
   SHELL_VAR *cb_return = NULL;
   SHELL_VAR *cb_row = NULL;
   SHELL_VAR *handle_array = NULL;

   static const char *MI_STEM = "PWLA_MAKE_KEY_";

   if ((retval = create_var_by_stem(&cb_return, MI_STEM, "make_key")))
      goto early_exit;

   if ((retval = create_array_var_by_stem(&cb_row, MI_STEM, "make_key")))
      goto early_exit;

   if ((retval = create_array_var_by_stem(&handle_array, MI_STEM, "make_key")))
      goto early_exit;

   // Make WORD_LIST for invoking callback function
   WORD_LIST *cb_head = NULL, *cb_tail = NULL;
   WL_APPEND(cb_tail, cb_return->name);
   cb_head = cb_tail;
   WL_APPEND(cb_tail, cb_row->name);
   // add extra parameters:
   ARG_LIST *extra = alist->next;
   while(extra)
   {
      WL_APPEND(cb_tail, extra->value);
      extra = extra->next;
   }

   // Prepare variables to collect results of callback function
   ARRAY *target_array = array_cell(handle_array);

   // For use with snprintf to stringify numbers for array elements
   char number_buffer[32];

   // Prepare pointers and limits for loop
   ARRAY_ELEMENT **ae_source = ahead->rows;
   ARRAY_ELEMENT **ae_end = ae_source + ahead->row_count;
   int row_index = 0;
   int array_index = 0;

   while (ae_source < ae_end)
   {
      // Fill the target row with current row contents
      if ((retval = update_row_array(cb_row, *ae_source, ahead->row_size)))
         goto early_exit;

      // Ask the caller how to save the row
      invoke_shell_function_word_list(function_var, cb_head);

      // Write the user's info to the new array:
      snprintf(number_buffer, sizeof(number_buffer), "%d", row_index);
      array_insert(target_array, array_index++, (char*)(cb_return->value));
      array_insert(target_array, array_index++, number_buffer);

      ++row_index;
      ++ae_source;
   }

   // Build a new head from the array resulting from callback results
   AHEAD *newhead = NULL;
   if (ate_create_indexed_head(&newhead, handle_array, 2))
   {
      qsort(newhead->rows,
            newhead->row_count,
            sizeof(ARRAY_ELEMENT*),
            pwla_make_key_qsort_callback);

      ate_dispose_variable_value(new_handle_var);
      new_handle_var->value = (char*)newhead;
      new_handle_var->attributes = att_special;

      printf("By now we should have a good handle named %s\n", new_handle_var->name);

      retval = EXECUTION_SUCCESS;
   }
   else
   {
      ate_register_error("unexpected failure indexing a new handle in 'make_key'");
      retval = EXECUTION_FAILURE;
   }

  early_exit:
   return retval;
}
