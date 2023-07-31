#include "pwla.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

/**
 * @brief Get index to key row whose value is equal to or greater than the target value
 *
 * This is a little convoluted, but it has to leave stuff to the
 * shell script because of our philosophy of minimal interface.
 *
 * In order to make it possible to select a range of rows, this
 * function returns an index into the key file.  That way, a script
 * can iterate through consecutive records if, for example, if there
 * are multiple equivalent key values.
 *
 * For example, if seek_key returns 10, you use get_row to get the
 * index 10 of the key table.  The key row will contain the key value
 * and the index into the target table where the record can be found.
 * Given the row index of 10 in the key table, the next key record
 * is 11.
 *
 * For now, it's a sequential search.  I'd like to implement a tree
 * search by comparing shifting half-way points according to previous
 * comparisons to compare as few records as possible.  If this is
 * implemented, we'll have to also search backwards from a match to
 * ensure the returned record is the first match.
 *
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 */
int pwla_seek_key(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *target_value = NULL;
   const char *value_name = NULL;

   ARG_TARGET append_data_targets[] = {
      { "handle_name", AL_ARG, &handle_name },
      { "target_value", AL_ARG, &target_value },
      { "v",            AL_OPT, &value_name},
      { NULL }
   };

   int retval = process_word_list_args(append_data_targets, alist, 0);
   if (retval)
      goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "seek_key")))
      goto early_exit;

   SHELL_VAR *value_var;
   if ((retval = create_var_by_given_or_default_name(&value_var,
                                                     value_name,
                                                     DEFAULT_VALUE_NAME,
                                                     "seek_key")))
      goto early_exit;

   retval = EX_USAGE;

   if (target_value == NULL)
   {
      ate_register_missing_argument("target_value", "seek_key");
      goto early_exit;
   }

   AHEAD *ahead = ahead_cell(handle_var);

   int ndx_left = 0;
   int ndx_right = ahead->row_count;
   int found = -1;

   while (found < 0)
   {
      ARRAY_ELEMENT **ael_ptr = &ahead->rows[ndx_left];
      ARRAY_ELEMENT **ael_end = &ahead->rows[ndx_right];
      while (ael_ptr < ael_end)
      {
         int comp = strcmp(target_value, (*ael_ptr)->value);

         // For exact match or if key is greater than the value
         if (comp <= 0)
         {
            found = (ael_ptr - ahead->rows);
            break;
         }

         ++ael_ptr;
      }

      // exhausted list without success:
      if (ael_ptr == ael_end)
      {
         ate_register_error("row not found");
         retval = EX_NOTFOUND;
         goto early_exit;
      }
   }

   if (found >= 0)
   {
      set_var_from_int(value_var, found);
      retval = EXECUTION_SUCCESS;
   }

  early_exit:
   return retval;
}
