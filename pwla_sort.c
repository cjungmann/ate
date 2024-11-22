/**
 * @file pwla_sort.c
 * @brief `sort` action implementation with qsort support function
 */

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <assert.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"
#include "pwla.h"
#include "word_list_stack.h"

struct sort_data {
   int row_size;
   SHELL_VAR *func_var;
   SHELL_VAR *return_var;
   SHELL_VAR *left_var;
   SHELL_VAR *right_var;
   WORD_LIST *args;
};

/**
 * @brief Transfer function between C-library qsort and a Bash shell comparison function
 * @param "left"  [in] pointer to left-side element
 * @param "right" [in] pointer to right-side element
 * @param "arg"   [in] void* to a @ref sort_data structure
 * @return -1 if left < right, 0 if left == right, 1 if left > right
 */
int pwla_sort_qsort_callback(const void *left, const void *right, void *arg)
{
   struct sort_data *data = (struct sort_data*)arg;

   // Prepre the rows for comparison
   ARRAY_ELEMENT *left_el = *(ARRAY_ELEMENT**)left;
   ARRAY_ELEMENT *right_el = *(ARRAY_ELEMENT**)right;
   assert(EXECUTION_SUCCESS == update_row_array(data->left_var, left_el, data->row_size));
   assert(EXECUTION_SUCCESS == update_row_array(data->right_var, right_el, data->row_size));

   invoke_shell_function_word_list(data->func_var, data->args);

   int compval = 0;
   get_int_from_string(&compval, data->return_var->value);

   return compval;
}

/**
 * @brief Create new handle with rows sorted via qsort
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 *
 * see man ate(1)
 */
int pwla_sort(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *callback_name = NULL;
   const char *new_handle_name = NULL;

   ARG_TARGET sort_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "callback_name", AL_ARG, &callback_name},
      { "new_handle_name", AL_ARG, &new_handle_name},
      { NULL }
   };

   int retval;

   // The following must be initialized because their values
   // will be checked for non-NULL upon an early exit
   SHELL_VAR *return_var = NULL, *left_var = NULL, *right_var = NULL;

   if ((retval = process_word_list_args(sort_targets, alist, AL_NO_OPTIONS)))
       goto early_exit;

   // Ignore a new handle name of --, which holds the argument
   // place to permit subsequent arguments to be passed to the
   // sorting callback function:
   if (new_handle_name && 0==strcmp(new_handle_name,"--"))
      new_handle_name=NULL;

   SHELL_VAR *handle_var = NULL;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "sort")))
      goto early_exit;

   SHELL_VAR *function_var = NULL;
   if ((retval = get_function_by_name_or_fail(&function_var,
                                              callback_name,
                                              "sort")))
      goto early_exit;

   const char *stem = "QSORT_ROW_STEM_";
   if ((retval = create_var_by_stem(&return_var, stem, "sort")))
      goto early_exit;
   if ((retval = create_array_var_by_stem(&left_var, stem, "sort")))
      goto early_exit;
   if ((retval = create_array_var_by_stem(&right_var, stem, "sort")))
      goto early_exit;

   // Prepare a copy of the function arguments for the qsort callback to use
   WORD_LIST *cb_args = NULL, *args_tail = NULL;
   WL_APPEND(args_tail, return_var->name);
   cb_args = args_tail;
   WL_APPEND(args_tail, left_var->name);
   WL_APPEND(args_tail, right_var->name);
   ARG_LIST *ptr = alist->next;
   while (ptr)
    {
      WL_APPEND(args_tail, ptr->value);
      ptr = ptr->next;
   }

   AHEAD *source_head = ahead_cell(handle_var);

   struct sort_data pkg = {
      source_head->row_size,
      function_var,
      return_var,
      left_var,
      right_var,
      cb_args
   };

   AHEAD *newhead = NULL;
   if (ate_create_indexed_head(&newhead, source_head->array, source_head->row_size))
   {
      qsort_r(&newhead->rows,
              newhead->row_count,
              sizeof(ARRAY_ELEMENT*),
              pwla_sort_qsort_callback,
              (void*)&pkg);

      if (new_handle_name)
      {
         SHELL_VAR *new_handle_var = NULL;
         if (ate_create_handle_with_head(&new_handle_var, new_handle_name, newhead))
            retval = EXECUTION_SUCCESS;
         else
         {
            xfree(newhead);
            ate_register_error("failed to initialize sorted handle, %s.", new_handle_name);
            retval = EXECUTION_FAILURE;
         }
      }
      else
         ate_install_head_in_handle(handle_var, newhead);
   }
   else
   {
      ate_register_error("failed to create indexed head for handle '%s'", new_handle_name);
      retval = EXECUTION_FAILURE;
   }

  early_exit:
   if (return_var)
      unbind_variable(return_var->name);
   if (left_var)
      unbind_variable(left_var->name);
   if (right_var)
      unbind_variable(right_var->name);

   return retval;
}
