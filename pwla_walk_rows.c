#include "pwla.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "pwla_walk_rows.h"

#include "word_list_stack.h"


/**
 * @brief Sends each row to a callback function
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 *
 * see man ate(1)
 */
int pwla_walk_rows(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *function_name = NULL;
   const char *key_handle_name = NULL;
   const char *start_ndx_str = NULL;
   const char *count_rows_str = NULL;

   ARG_TARGET walk_rows_targets[] = {
      { "handle_name",   AL_ARG, &handle_name},
      { "function_name", AL_ARG, &function_name},
      { "k"            , AL_OPT, &key_handle_name},
      { "s"            , AL_OPT, &start_ndx_str},
      { "c"            , AL_OPT, &count_rows_str},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(walk_rows_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_key_var = NULL;
   SHELL_VAR *handle_var = NULL;

   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "walk_rows")))
      goto early_exit;

   if (key_handle_name)
   {
      if ((retval = get_handle_var_by_name_or_fail(&handle_key_var,
                                                   key_handle_name,
                                                   "walk_rows")))
         goto early_exit;
   }


   AHEAD *walker_ahead = NULL, *data_ahead = NULL;

   if (handle_key_var)
   {
      walker_ahead = ahead_cell(handle_key_var);
      data_ahead = ahead_cell(handle_var);
   }
   else
      walker_ahead = ahead_cell(handle_var);

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
   int count_rows = walker_ahead->row_count;

   // Sanity checks
   if (start_ndx_str)
   {
      if (get_int_from_string(&start_ndx, start_ndx_str))
      {
         if (start_ndx < 0 || start_ndx >= walker_ahead->row_count)
         {
            ate_register_invalid_row_index(start_ndx, walker_ahead->row_count);
            goto early_exit;
         }
      }
      else
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
   if (start_ndx + count_rows > walker_ahead->row_count)
      count_rows = walker_ahead->row_count - start_ndx;

   /***  Make a reusable WORD_LIST for calling the callback function ***/

   WORD_LIST *cb_args = NULL, *cb_tail = NULL;
   // #1 argument is the array name
   WL_APPEND(cb_tail, array_var->name);
   cb_args = cb_tail;

   // #2 argument is row_index
   // setup row_index WORD_DESC with a buffer we can change each iteration:
   char row_number_buffer[32];
   WL_APPEND(cb_tail, "");
   cb_tail->word->word = row_number_buffer;

   // #3 argument is the source handle's name
   WL_APPEND(cb_tail, handle_name);

   // #4 argument is order index, may be same as row_index:
   char order_number_buffer[32];
   WL_APPEND(cb_tail, "");
   cb_tail->word->word = order_number_buffer;

   // add extra parameters:
   ARG_LIST *extra = alist->next;
   while(extra)
   {
      WL_APPEND(cb_tail, extra->value);
      extra = extra->next;
   }

   ARRAY_ELEMENT **ae_ptr = &walker_ahead->rows[start_ndx];
   ARRAY_ELEMENT **ae_end = ae_ptr + count_rows;

   // Track current row index for callback parameter
   int row_ndx, order_ndx, cur_ndx = start_ndx;
   ARRAY_ELEMENT *ae_row;

   int data_row_size = data_ahead ? data_ahead->row_size : walker_ahead->row_size;

   while (ae_ptr < ae_end)
   {
      if (data_ahead)
      {
         const char *ndx_str = (*ae_ptr)->next->value;

         // In ordered-walk, we'll need to
         // set both indexes individually:
         order_ndx = cur_ndx;
         if (!get_int_from_string(&row_ndx, ndx_str))
         {
            ate_register_error("field value '%s' in table '%s' is not a key row index in walk_rows",
                               ndx_str, key_handle_name);
            goto early_exit;
         }
         ae_row = data_ahead->rows[row_ndx];
      }
      else
      {
         ae_row = *ae_ptr;
         // natural order-walk, order and row index the same:
         order_ndx = row_ndx = cur_ndx;
      }

      // For next iteration
      ++cur_ndx;

      // Setup row_index and order_index WORD_DESC values for this iteration
      snprintf(row_number_buffer, sizeof(row_number_buffer), "%d", row_ndx);
      snprintf(order_number_buffer, sizeof(order_number_buffer), "%d", order_ndx);

      // Fill the target row with current row contents
      if ((retval = update_row_array(array_var, ae_row, data_row_size)))
         goto early_exit;

      // Prepare and call the callback
      if ((0 != invoke_shell_function_word_list(function_var, cb_args)))
         goto early_exit;

      ++ae_ptr;
   }

   retval = EXECUTION_SUCCESS;

  early_exit:
   return retval;
}

