#include "pwla.h"

#include <stdio.h>
#include <assert.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

typedef int (*pwla_comp_func)(const char*, const char*);

/**
 * @brief Ultimate determinor of comparison function to use by @ref pwla_seek_key
 *
 * This pointer is either directly referenced by the funtion pointer
 * @r pcomp in @ref pwla_seek_key, or is called by the #ref tally_comp
 * global function after it increments the comparison count.
 */
pwla_comp_func pwla_sort_func = strcmp;

/**
 * @brief This variable must be global to ensure it's available
 *        to @ref tally_comp without having to add another parameter
 *        to the p
 */
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
   return (*pwla_sort_func)(left,right);
}

int debug_comp(const char *left, const char *right)
{
   ++pwla_comp_tally;
   int result = (*pwla_sort_func)(left, right);
   if (result < 0)
      printf("%3d: pivot '%s' is less than target, search to right.\n",
             pwla_comp_tally, left);
   else if (result > 0)
      printf("%3d: pivot '%s' is greater than target, search to left.\n",
             pwla_comp_tally, left);
   else
      printf("%3d: pivot '%s' is equal to the target.\n",
             pwla_comp_tally, left);

   return result;
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
   const char *debug_flag = NULL;
   const char *int_sort_flag = NULL;
   const char *permissive_flag = NULL;
   const char *sequential_flag = NULL;

   ARG_TARGET append_data_targets[] = {
      { "handle_name",  AL_ARG, &handle_name },
      { "search_value", AL_ARG, &search_value },
      { "v",            AL_OPT, &value_name},
      { "t",            AL_OPT, &comp_tally_name },
      { "d",            AL_FLAG, &debug_flag},
      { "i",            AL_FLAG, &int_sort_flag},
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

   /**
    * @brief int_sort_flag must be determined before comp_tally_name test
    * 
    * The following two tests (int_sort_flag and comp_tally_name) work
    * together to set the sorting method.  The int_sort_flag must be
    * handled first because the result of comp_tally_name sets the
    * value of @ref pcomp to the value of @ref pwla_sort_func just
    * before testing for a request to tally comparisons.  The integer
    * sort request would be ignored if the test occurred after the
    * comp_tally_name test.
    */

   if (int_sort_flag)
      pwla_sort_func = int_strcmp;
   else
      pwla_sort_func = strcmp;

   pwla_comp_func pcomp = pwla_sort_func;
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

   // Set or override tally flag to use debug-reporting version of comparison:
   if (debug_flag)
   {
      pwla_comp_tally = 0;
      pcomp = debug_comp;
      printf("\nDebug search for first instance of '%s'\n", search_value);
   }



   int permissive_match = permissive_flag==NULL ? 0 : 1;
   int sequential_search = sequential_flag==NULL ? 0 : 1;

   AHEAD *ahead = ahead_cell(handle_var);

   int ndx_left = 0;
   int ndx_right = ahead->row_count;

   int binary_search = 1;
   int linear_threshhold = 3;

   ARRAY_ELEMENT **ael_ptr, **ael_end;

   // Quick and dirty for sequential sort, then skip to exit.
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

   // Default search is binary search to a small range, then
   // linear search to find key equal to or greater than the
   // search value:
   while (1)
   {
      if (ndx_right - ndx_left <= linear_threshhold)
         binary_search = 0;

      if (binary_search)
      {
         int mid = (ndx_left + ndx_right) / 2;
         ael_ptr = &ahead->rows[mid];

         // NOTE: it would be more useful, in debug_mode, to show
         //       the index of the pivot element.  I choose not to
         //       do that because that would require a per-iteration
         //       conditional.  I am using a function pointer for
         //       the comparison expressly to avoid a per-iteration
         //       condition test.  Using debug_mode is likely to be
         //       rare, so having debug_mode available shouldn't
         //       penalize typical use.
         //
         //       Other debug_mode output occurs once per call to
         //       seek_key, so I'm leaving those in.
         //
         //       If we change our mind, uncomment this:
         // if (debug_mode)
         //    printf("key pivot index %d: ", mid);

         int comp = (*pcomp)((*ael_ptr)->value, search_value);
         if (comp >= 0)
            ndx_right = mid;
         else
            ndx_left = mid;
      }
      else // conduct linear search through remaining elements
      {
         // After binary search finds last value smaller than the target,
         // the next value should be row that is equal to or greater than
         // the requested value

         ael_ptr = &ahead->rows[ndx_left];
         ael_end = &ahead->rows[ndx_right];

         if (debug_flag)
         {
            // Remember that ael_end is the unused pointer PAST
            // the last considered element.  If we want to show
            // a limit, we need to back-off one element (ael_end-1).
            printf("begin sequential search from '%s' to '%s'\n",
                   (*ael_ptr)->value, (*(ael_end-1))->value);
         }

         while (ael_ptr < ael_end)
         {
            int comp = (*pcomp)((*ael_ptr)->value, search_value);

            if (comp==0)
               goto found_value;

            // First greater-than match should be the search_string
            // or a string just after it:
            if (comp > 0)
            {
               // test for exact match if necessary
               if (permissive_match)
                  goto found_value;
               else
                  goto giving_up;
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
      } // end of linear search code block
   }

  giving_up:
   if (debug_flag)
      printf("\x1b[31;1mGave up searching\x1b[m for '%s'\n", search_value);

   ate_register_error("row not found");
   retval = EX_NOTFOUND;
   goto save_tally;

  found_value:
   {
      if (debug_flag)
      {
         char *found_value = (*ael_ptr)->value;

         int comp = strcmp(found_value, search_value);
         if (comp < 0)
            printf("\x1b[31;1mUnacceptable result, '%s' is not equal to or greater than '%s'.\n",
                   found_value, search_value);
         else
         {
            int color = ( comp == 0 ) ? 32 : 33;
            printf("\x1b[%d;1mAccepting '%s' as acceptable result in search for '%s'\x1b[m\n",
                   color, found_value, search_value);
         }
      }

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
