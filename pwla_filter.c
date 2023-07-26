/* -*- mode: c -*- */

#include "pwla.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

/**
 * @brief FAKE ACTION filter
 */
int pwla_filter(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *function_name = NULL;
   const char *new_handle_name = NULL;

   ARG_TARGET filter_targets[] = {
      { "handle_name",     AL_ARG, &handle_name},
      { "callback_name",   AL_ARG, &function_name},
      { "new_handle_name", AL_ARG, &new_handle_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(filter_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "filter")))
      goto early_exit;

   SHELL_VAR *callback_var;
   if ((retval = get_function_by_name_or_fail(&callback_var,
                                              function_name,
                                              "filter")))
      goto early_exit;

   retval = EX_USAGE;

   if (new_handle_name == NULL)
   {
      ate_register_missing_argument("new handle name", "filter");
      goto early_exit;
   }

   // For actions that create an array for callback functions
   SHELL_VAR *new_array = NULL;
   if ((retval = create_array_var_by_stem(&new_array, "ATE_FILTER_ARRAY_", "template")))
      goto early_exit;

   // Make WORD_LIST of args for each call
   WORD_LIST *args = NULL, *tail = NULL;
   WL_APPEND(tail, new_array->name);
   args = tail;
   ARG_LIST *argptr = alist->next;
   while (argptr)
   {
      WL_APPEND(tail, argptr->value);
      argptr = argptr->next;
   }

   // Use the values aquired above
   AHEAD *ahead = ahead_cell(handle_var);
   ARRAY_ELEMENT **ptr = ahead->rows;
   ARRAY_ELEMENT **end = ptr + ahead->row_count;

   // Run the filter
   AEL *ael_list = NULL, *ael_tail = NULL, *ael_cur = NULL;
   int count = 0;
   while (ptr < end)
   {
      // Update new_array with current row contents:
      if ((retval = update_row_array(new_array, *ptr, ahead->row_size)))
         goto early_exit;

      if (EXECUTION_SUCCESS == invoke_shell_function_word_list(callback_var, args))
      {
         ++count;
         ael_cur = (AEL*)alloca(sizeof(AEL));
         ael_cur->element = *ptr;
         ael_cur->next = NULL;
         if (ael_tail)
            ael_tail->next = ael_cur;
         else
            ael_list = ael_cur;

         ael_tail = ael_cur;
      }

      ++ptr;
   }

   AHEAD *new_head = NULL;

   retval = EXECUTION_FAILURE;

   if (ate_create_head_with_ael(&new_head,
                                ahead->array,
                                ahead->row_size,
                                count,
                                ael_list))
   {
      SHELL_VAR *var = NULL;
      if (ate_install_head_in_handle(&var,
                                     new_handle_name,
                                     new_head))
         retval = EXECUTION_SUCCESS;
      else
         xfree(new_head);
   }

  early_exit:
   return retval;
}

