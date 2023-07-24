
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

#define QSORT_VAR_STEM "ATE_QSORT_VAR_"

int action_sort_qsort_callback(const void *left, const void *right, void *arg)
{
   int exitval;
   struct qsort_package *pkg = (struct qsort_package*)arg;

   SHELL_VAR *sv_left = NULL;
   exitval = clone_range_to_array(&sv_left,
                                  *(ARRAY_ELEMENT**)left,
                                  pkg->row_size,
                                  pkg->name_left);

   assert(exitval==EXECUTION_SUCCESS);


   SHELL_VAR *sv_right = NULL;
   exitval = clone_range_to_array(&sv_right,
                                  *(ARRAY_ELEMENT**)right,
                                  pkg->row_size,
                                  pkg->name_right);
   assert(exitval==EXECUTION_SUCCESS);

   if (invoke_shell_function(pkg->callback_func,
                             pkg->return_var->name,
                             pkg->name_left,
                             pkg->name_right,
                             NULL))
   {
      printf("Returned a failure (sob).\n");
      return 0;
   }
   else
      return atoi(pkg->return_var->value);;
}

/**
 * @brief Make a sorted index
 * @param "name_handle"   initialized ate handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         extra[0] should be sort function name
 *                        extra[1] should be the handle name to use
 *
 * @return EXECUTION_SUCCESS
 */
int ate_action_sort(const char *name_handle,
                    const char *name_value,
                    const char *name_array,
                    WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle, "sort");
   if (retval)
      goto early_exit;

   const char *callback_name = NULL;
   SHELL_VAR *callback_func = NULL;
   if (get_string_from_list(&callback_name, extra, 0))
   {
      callback_func = find_function(callback_name);
      if (callback_func == NULL)
      {
         ate_register_function_not_found(callback_name);
         retval = EX_USAGE;
         goto early_exit;
      }
   }
   else
   {
      ate_register_missing_argument("sorting callback function", "sort");
      retval = EX_USAGE;
      goto early_exit;
   }

   // A name for a new handle variable is required
   const char *name_new_handle = NULL;
   if (!get_string_from_list(&name_new_handle, extra, 1))
   {
      ate_register_missing_argument("sorted handle name", "sort");
      retval = EX_USAGE;
      goto early_exit;
   }

   AHEAD *head = NULL;
   if (ate_create_indexed_head(&head, handle->array, handle->row_size))
   {
      static const char name_stem[] = QSORT_VAR_STEM;
      static const int buffer_len = 4 + sizeof QSORT_VAR_STEM;

      char *name_return = (char*)alloca(buffer_len);
      char *name_left = (char*)alloca(buffer_len);
      char *name_right = (char*)alloca(buffer_len);

      __attribute((unused)) SHELL_VAR *sv_return, *sv_left, *sv_right;

      // Bind names to variables or the name won't be reserved
      make_unique_name(name_return, buffer_len, name_stem);
      sv_return = bind_variable(name_return, "", 0);
      make_unique_name(name_left, buffer_len, name_stem);
      sv_left = make_new_array_variable(name_left);
      make_unique_name(name_right, buffer_len, name_stem);
      sv_right = make_new_array_variable(name_right);

      struct qsort_package pkg = {
         head->row_size,
         callback_func,
         bind_variable(name_return, "", 0),
         name_left,
         name_right
      };

      qsort_r(&head->rows,
              head->row_count,
              sizeof(ARRAY_ELEMENT*),
              action_sort_qsort_callback,
              (void*)&pkg);

      // Discard variables only needed during qsort_r
      unbind_variable(name_return);
      unbind_variable(name_right);
      unbind_variable(name_left);
      unbind_variable(name_return);

      if (ate_install_head_in_handle(NULL, name_new_handle, head))
         retval = EXECUTION_SUCCESS;
      else
      {
         xfree(head);
         ate_register_error("failed to create sorted handle '%s'", name_new_handle);
         retval = EXECUTION_FAILURE;
      }
   }

  early_exit:
   return retval;
}

