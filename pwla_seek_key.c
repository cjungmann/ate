#include "pwla.h"

#include <stdio.h>
#include <assert.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

typedef int (*pwla_comp_func)(const char*, const char*);

static int pwla_comp_tally=0;
/**
 * @brief Drop-in replacement for strcmp() in order to count the
 *        number of comparisons performed.
 * @param "left"   left-side value to compare
 * @param "right"  right-side value to compare
 * @return <0 if left is less than right, >0 if left if greater than
 *         right, 0 if equal
 */
int tally_comp(const char *left, const char *right)
{
   ++pwla_comp_tally;
   return strcmp(left,right);
}

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
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 */
int pwla_seek_key(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *search_value = NULL;
   const char *value_name = NULL;
   const char *comp_tally_name = NULL;
   const char *permissive_flag = NULL;
   const char *sequential_flag = NULL;

   ARG_TARGET append_data_targets[] = {
      { "handle_name",  AL_ARG, &handle_name },
      { "search_value", AL_ARG, &search_value },
      { "v",            AL_OPT, &value_name},
      { "t",            AL_OPT, &comp_tally_name },
      { "p",            AL_FLAG, &permissive_flag},
      { "s",            AL_FLAG, &sequential_flag },
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

   if (search_value == NULL)
   {
      ate_register_missing_argument("search_value", "seek_key");
      retval = EX_USAGE;
      goto early_exit;
   }

   pwla_comp_func pcomp = strcmp;
   SHELL_VAR *tally_var = NULL;
   if (comp_tally_name)
   {
      if ((retval = create_var_by_given_or_default_name(&tally_var,
                                                        comp_tally_name,
                                                        NULL,
                                                        "seek_key")))
         goto early_exit;

      pcomp = tally_comp;
      pwla_comp_tally = 0;
   }

   int permissive_match = permissive_flag==NULL ? 0 : 1;
   int sequential_search = sequential_flag==NULL ? 0 : 1;

   AHEAD *ahead = ahead_cell(handle_var);

   int ndx_left = 0;
   int ndx_right = ahead->row_count;

   int binary_search = 1;
   int linear_threshhold = 3;

   ARRAY_ELEMENT **ael_ptr, **ael_end;

   // Quick and dirty for sequential sort
   if (sequential_search)
   {
      ael_ptr = ahead->rows;
      ael_end = ael_ptr + ahead->row_count;
      while (ael_ptr < ael_end)
      {
         if (0 == (*pcomp)((*ael_ptr)->value, search_value))
            goto found_value;
         ++ael_ptr;
      }
      goto giving_up;
   }

   // Make an untypeable value (so it can't ever match) that is
   // less than the search string but for which no value can exist
   // betwwen it and the search string.  This should guarantee that
   // it will match the first instance of the search string in a
   // sorted list.
   int target_len = strlen(search_value);
   char *target_value = (char*)(alloca(target_len+2));
   memcpy(target_value, search_value, target_len);
   // decrement last letter, append highest possible, untypeable char
   --target_value[target_len-1];
   target_value[target_len] = (char)255;
   target_value[target_len+1] = '\0';

   while (1)
   {
      if (ndx_right - ndx_left <= linear_threshhold)
         binary_search = 0;

      if (binary_search)
      {
         int mid = (ndx_left + ndx_right) / 2;
         ael_ptr = &ahead->rows[mid];
         int comp = (*pcomp)((*ael_ptr)->value, target_value);

         // We should NEVER match because we should have made
         // an unmatchable value that is the last value before
         // the search string...but in case I'm wrong:
         assert(comp);

         if (comp > 0)        // target after node, search right
            ndx_right = mid;
         else if (comp < 0)   // target before node, search left
            ndx_left = mid;
      }
      else // conduct linear search through remaining elements
      {
         ael_ptr = &ahead->rows[ndx_left];
         ael_end = &ahead->rows[ndx_right];

         while (ael_ptr < ael_end)
         {
            int comp = (*pcomp)((*ael_ptr)->value, target_value);

            // Again, for the same reason describe above, we
            // should never match:
            assert(comp);

            // First greater-than match should be the search_string
            // or a string just after it:
            if (comp > 0)
            {
               // test for exact match if necessary
               if (!permissive_match)
               {
                  comp = (*pcomp)((*ael_ptr)->value, search_value);
                  if (comp)
                     goto giving_up;
               }

               goto found_value;
            }

            ++ael_ptr;
         }

         // exhausted list without exact match:
         if (ael_ptr == ael_end)
         {
            // If we're at the end of the entire table, we've failed,
            if (ael_ptr == ahead->rows+ahead->row_count)
               goto giving_up;

            goto found_value;
         }
      }
   }

  giving_up:
   ate_register_error("row not found");
   retval = EX_NOTFOUND;
   goto save_tally;

  found_value:
   {
      set_var_from_int(value_var, ael_ptr - ahead->rows);
      retval = EXECUTION_SUCCESS;
   }

  save_tally:
   // Keep promise to report tally count:
   if (tally_var)
      set_var_from_int(tally_var, pwla_comp_tally);

  early_exit:
   return retval;
}
