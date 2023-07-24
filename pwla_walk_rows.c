#include "process_word_list_args.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

/**
 * @brief FAKE ACTION walk_rows
 */
int pwla_walk_rows(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *function_name = NULL;
   const char *start_ndx_str = NULL;
   const char *count_rows_str = NULL;

   ARG_TARGET walk_rows_targets[] = {
      { "handle_name",   AL_ARG, &handle_name},
      { "function_name", AL_ARG, &function_name},
      { "s"            , AL_OPT, &start_ndx_str},
      { "c"            , AL_OPT, &count_rows_str},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(walk_rows_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "walk_rows")))
      goto early_exit;

   AHEAD *ahead = ahead_cell(handle_var);

   SHELL_VAR *function_var = NULL;
   if ((retval = get_function_by_name_or_fail(&function_var,
                                              function_name,
                                              "walk_rows")))
      goto early_exit;

   // Array to pass to callback
   SHELL_VAR *array_var;
   if ((retval = create_array_var_by_stem(&array_var, "ATE_WALKING_ROW_", "walk_rows")))
      goto early_exit;

   retval = EX_USAGE;

   int start_ndx = 0;
   int count_rows = ahead->row_count;

   if (start_ndx_str)
   {
      if (! get_int_from_string(&start_ndx, start_ndx_str))
      {
         ate_register_not_an_int(start_ndx_str, "walk_rows");
         goto early_exit;
      }
   }

   if (count_rows_str)
   {
      if (! get_int_from_string(&count_rows, count_rows_str))
      {
         ate_register_not_an_int(start_ndx_str, "walk_rows");
         goto early_exit;
      }
   }

   // Fix overreach
   if (start_ndx + count_rows > ahead->row_count)
      count_rows = ahead->row_count - start_ndx;

   /***  Make a reusable WORD_LIST for calling the callback function ***/

   WORD_LIST *cb_args = NULL, *cb_tail = NULL;
   // first argument is the array name
   WL_APPEND(cb_tail, array_var->name);
   cb_args = cb_tail;
   // second argument is row_index
   // setup row_index WORD_DESC with a buffer we can change each iteration:
   char row_number_buffer[32];
   WL_APPEND(cb_tail, "");
   cb_tail->word->word = row_number_buffer;
   // third argument is the source handle's name
   WL_APPEND(cb_tail, handle_name);
   // add extra parameters:
   ARG_LIST *extra = alist->next;
   while(extra)
   {
      WL_APPEND(cb_tail, extra->value);
      extra = extra->next;
   }

   ARRAY_ELEMENT **ae_ptr = &ahead->rows[start_ndx];
   ARRAY_ELEMENT **ae_end = ae_ptr + count_rows;

   // Track current row index for callback parameter
   int cur_ndx = start_ndx;

   while (ae_ptr < ae_end)
   {
      // Setup row_index WORD_DESC value for this iteration
      snprintf(row_number_buffer, sizeof(row_number_buffer), "%d", cur_ndx++);

      // Fill the target row with current row contents
      if ((retval = update_row_array(array_var, *ae_ptr, ahead->row_size)))
         goto early_exit;

      // Prepare and call the callback
      invoke_shell_function_word_list(function_var, cb_args);

      ++ae_ptr;
   }

   retval = EXECUTION_SUCCESS;

  early_exit:
   return retval;
}

