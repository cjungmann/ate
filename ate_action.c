/**
 * @file ate_action.c
 * @brief Functions callable through the `ate` interface.
 */

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "ate_action.h"
#include "ate_utilities.h"
#include "ate_errors.h"



/*****
 * @brief TEMPLATE
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         ignored
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */

/**
 * @brief Show list of available actions
 * @param "name_handle"   ignored
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         ignored
 *
 * @return EXECUTION_SUCCESS
 */
int ate_action_list_actions(const char *name_handle,
                            const char *name_value,
                            const char *name_array,
                            WORD_LIST *extra)
{
   delegate_list_actions();
   return EXECUTION_SUCCESS;
}

/**
 * @brief Display help screen
 *
 * This function repurposes the @p name_handle parameter to indicate
 * for which specific action the command should display a more
 * detailed help screen.  If @p name_handle is blank, the detailed
 * help screen for each action will be displayed.
 * 
 * @param "name_handle"   reinterpreted as a requested action name
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         ignored
 * @return EXECUTION_SUCCESS
 */
int ate_action_show_action(const char *name_handle,
                           const char *name_value,
                           const char *name_array,
                           WORD_LIST *extra)
{
   delegate_show_action_usage(name_handle);
   return EXECUTION_SUCCESS;
}

/**
 * @brief Create a new ate handle.
 * @param "name_handle"   name of handle to create
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         may include zero, one. or two options:
 *                        - the first number argument will be taken as
 *                          the row_size value.
 *                        - the first non-number argument will be taken
 *                          as the desired array name.  If the array
 *                          already exists, it will be confirmed as an
 *                          array and used if appropriate.  If the
 *                          variable doesn't exist, it will be created
 *                          as an array.
 *
 * If the array name is not specified, an Bash array variable will
 * be created with a unique name.  If the row size is not specified,
 * the row size will be set to 1.
 *
 * Errors if a variable with the handle name already exists, or if
 * an invalid row size is requested (less than 1), or if a requested
 * array name exists and is not an array.
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_declare(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   int retval = EXECUTION_FAILURE;

   if (find_variable(name_handle))
   {
      retval = ate_error_handle_already_exists(name_handle);
      goto early_exit;
   }

   // Stuff we need for fully-functioning handle initialized
   // to recognizably unset values
   SHELL_VAR* hosted_array = NULL;
   int row_size = -1;

   const char *cur_word;
   char *end_word;
   int cur_size = 0;

   // The first number found will be the row_size, and the
   // first non-number string will be the array name.
   WORD_LIST *ptr = extra;
   while (ptr)
   {
      cur_word = ptr->word->word;

      cur_size = strtol(cur_word, &end_word, 10);
      if (end_word > cur_word)
      {
         // Consider number value only if row_size is not yet set
         if (row_size < 1)
         {
            if (cur_size < 1)
            {
               retval = ate_error_invalid_row_size(cur_size);
               goto early_exit;
            }
            else
               row_size = cur_size;
         }
         else
            goto continue_next;
      }
      else if (hosted_array == NULL)
      {
         SHELL_VAR *var = find_variable(cur_word);
         if (var)
         {
            if (array_p(var))
               hosted_array = var;
            else
            {
               retval = ate_error_wrong_type_var(var, "array");
               goto early_exit;
            }
         }
         // Create requested array by name
         else
            hosted_array = make_new_array_variable((char*)cur_word);
      }

     continue_next:
      ptr = ptr->next;
   }

   // Handle missing row_size
   if (row_size == -1)
      row_size = 1;

   // Handle missing array name
   if (hosted_array == NULL)
   {
      static const char *stem = "ATE_HOSTED_ARRAY_";

      int buff_len = strlen(stem) + 5;
      char *name = (char*)alloca(buff_len);
      if (name && make_unique_name(name, buff_len, stem))
         hosted_array = make_new_array_variable(name);

      if (hosted_array == NULL)
      {
         retval = ate_error_failed_to_make_handle("new array failure");
         goto early_exit;
      }
   }

   // We have all the parts, construct the SHELL_VAR host:

   // Check for and announce otherwise silent error during ate_create_handle
   int el_count = array_num_elements(array_cell(hosted_array));
   if ( el_count % row_size)
   {
      retval = ate_error_invalid_row_size(row_size);
      goto early_exit;
   }

   SHELL_VAR *hvar = NULL;
   if (ate_create_handle(&hvar, name_handle, hosted_array, row_size))
      retval = EXECUTION_SUCCESS;
   else
   {
      dispose_variable(hosted_array);
      retval = EXECUTION_FAILURE;
   }

  early_exit:
   return retval;
}


/**
 * @brief Returns number of indexed rows.
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    variable name override, default `ATE_VALUE`
 * @param "name_array"    ignored
 * @param "extra"         ignored
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_get_row_count(const char *name_handle,
                             const char *name_value,
                             const char *name_array,
                             WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval == EXECUTION_SUCCESS)
   {
      SHELL_VAR *var = ate_get_prepared_variable(name_value, att_special);
      if (var)
         retval = set_var_from_int(var, handle->row_count);
      else
         retval = EX_NOTFOUND;
   }

   return retval;
}

/**
 * @brief Returns currently-set number of fields in a virtual row.
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    variable name override, default `ATE_VALUE`
 * @param "name_array"    ignored
 * @param "extra"         ignored
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_get_row_size(const char *name_handle,
                              const char *name_value,
                              const char *name_array,
                              WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval == EXECUTION_SUCCESS)
   {
      SHELL_VAR *var = ate_get_prepared_variable(name_value, att_special);
      if (var)
         retval = set_var_from_int(var, handle->row_size);
      else
         retval = EX_NOTFOUND;
   }

   return retval;
}

/**
 * @brief Returns script-accessible name of the hosted array variable.
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    variable name override, default `ATE_VALUE`
 * @param "name_array"    ignored
 * @param "extra"         ignored
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_get_array_name(const char *name_handle,
                              const char *name_value,
                              const char *name_array,
                              WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval)
      goto early_exit;

   SHELL_VAR *svar = find_variable(name_value);
   if (svar == NULL)
      svar = bind_variable(name_value,"",0);

   if (svar)
   {
      const char *name = handle->array->name;
      if (name)
      {
         ate_dispose_variable_value(svar);
         svar->value = savestring(name);
         retval = EXECUTION_SUCCESS;
      }
   }
   else
      retval = ate_error_failed_to_create(name_value);

  early_exit:
   return retval;
}

/**
 * @brief Returns an array containing contents of referenced row.
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    array name override, default `ATE_ARRAY`
 * @param "extra"         extra[0] is requested row index (0-based)
 *
 * This function fills a small array with the contents of the
 * virtual row indicated by the row index.  Changes to the contents
 * of the result array will not be written back to the source table
 * unless it is submitted back with @ref ate_action_put_row.
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_get_row(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval)
      goto early_exit;

   // Prepare resources (valid row index, source row, and clean return array)
   int requested_index;
   if (! get_int_from_list(&requested_index, extra, 0))
   {
      retval = ate_error_missing_usage("row index in get_row");
      goto early_exit;
   }

   if (requested_index < 0 && requested_index >= handle->row_count)
   {
      retval = ate_error_record_out_of_range(requested_index, handle->row_count);
      goto early_exit;
   }

   ARRAY_ELEMENT *arel = ate_get_indexed_row(handle, requested_index);
   if (arel == NULL)
   {
      retval = ate_error_record_out_of_range(requested_index, handle->row_count);
      goto early_exit;
   }

   SHELL_VAR *return_array;
   if (!prepare_clean_array_var(&return_array, name_array))
   {
      retval = ate_error_failed_to_create("a return array");
      goto early_exit;
   }

   // Copy source row to return array

   assert(retval == EXECUTION_SUCCESS);

   ARRAY *arr = array_cell(return_array);

   // declared outside of `for` statement to preserve its value
   int i;

   // The row contents in the following elements
   for (i=0; i<handle->row_size && arel; ++i)
   {
      array_insert(arr, i, arel->value);
      arel = arel->next;
   }

   // Abort and free resources if incomplete row
   if (i < handle->row_size)
   {
      array_dispose(arr);
      retval = ate_error_corrupt_table();
   }


  early_exit:
   return retval;
}

/**
 * @brief Updates a table row with the contents of an array
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         extra[0] is requested row index (0-based)
 *                        extra[1] is the name of the source array
 *
 * This function is meant for updating a row that was returned by
 * the `get_row` action.
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_put_row(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval)
      goto early_exit;

   // Collect resources for the action
   int fields_to_copy = handle->row_size;

   // Prepare resources (valid row index, target row, and source array)
   int requested_index;
   if (! get_int_from_list(&requested_index, extra, 0))
   {
      retval = ate_error_missing_usage("row index in get_row");
      goto early_exit;
   }

   if (requested_index < 0 && requested_index >= handle->row_count)
   {
      retval = ate_error_record_out_of_range(requested_index, handle->row_count);
      goto early_exit;
   }

   ARRAY_ELEMENT *target_arel = ate_get_indexed_row(handle, requested_index);
   if (target_arel == NULL)
   {
      retval = ate_error_record_out_of_range(requested_index, handle->row_count);
      goto early_exit;
   }

   // Get source array variable, terminate for problem.
   SHELL_VAR *source_array_var = NULL;
   const char *array_name = NULL;
   // Find array name
   if (!get_string_from_list(&array_name, extra, 1))
   {
      retval = ate_error_missing_usage("source array name");
      goto early_exit;
   }

   // find array_name SHELL_VAR
   source_array_var = find_variable(array_name);
   if (source_array_var == NULL)
   {
      retval = ate_error_var_not_found(array_name);
      goto early_exit;
   }

   // Confirm SHELL_VAR is an array
   if (!array_p(source_array_var))
   {
      retval = ate_error_wrong_type_var(source_array_var, "array");
      goto early_exit;
   }

   ARRAY *source_array = array_cell(source_array_var);

   // Confirm match row_sizes
   if (source_array->num_elements != fields_to_copy)
   {
      retval = ate_error_mismatched_row_size(source_array_var, fields_to_copy);
      goto early_exit;
   }

   //
   // We've validated the data, begin the update:
   //

   ARRAY_ELEMENT *source_head = array_head(source_array);
   ARRAY_ELEMENT *source_arel = source_head->next;

   assert(retval == EXECUTION_SUCCESS);

   // We have the necessary resource, begin:
   for (int i=0; i<fields_to_copy; ++i)
   {
      // The target row should be long enough to copy fields_to_copy
      assert(target_arel);

      free(target_arel->value);

      // If source is short, fill remaining target with empty values
      if (source_arel)
      {
         target_arel->value = savestring(source_arel->value);
         source_arel = source_arel->next;
      }
      else
         target_arel->value = "";

      target_arel = target_arel->next;
   }

  early_exit:
   return retval;
}

/**
 * @brief Adds table rows from a list of arguments
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         list of arguments
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 *
 * This function can be used for bulk insert of data.  Each extra
 * positional argument will be added to a buffer array until the
 * buffer contains a full row's data.  When the row buffer is full,
 * a new row will be appended to the hosted array.
 *
 * An incomplete buffer will not be appended to the hosted array.
 */
int ate_action_append_data(const char *name_handle,
                           const char *name_value,
                           const char *name_array,
                           WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval == EXECUTION_SUCCESS)
   {
      ARRAY *array = array_cell(handle->array);
      int next_index = array->max_index + 1;

      int row_size = handle->row_size;
      char **row_buffer = (char**)alloca(row_size * sizeof(char*));
      char **row_ptr = row_buffer;
      char **row_end = row_buffer + row_size;

      WORD_LIST *ptr = extra;
      while (ptr)
      {
         *row_ptr = ptr->word->word;
         ++row_ptr;
         if (row_ptr >= row_end)
         {
            char **copy_ptr = row_buffer;
            while (copy_ptr < row_end)
            {
               array_insert(array, next_index++, *copy_ptr);
               ++copy_ptr;
            }
            row_ptr = row_buffer;
         }
         ptr = ptr->next;
      }
   }

   return retval;
}

/**
 * @brief Generate a new index of table row heads
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         ignored
 *
 * Generate a new index of table row heads which, if successful
 * will replace the existing table row heads index.  The heap memory
 * hosting the existing table row heads will be 'free'd.
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_index_rows (const char *name_handle, const char *name_value,
                           const char *name_array, WORD_LIST *extra)
{
   int retval = EXECUTION_FAILURE;

   SHELL_VAR *shandle = find_variable(name_handle);
   if (shandle == NULL)
   {
      retval = ate_error_var_not_found(name_handle);
      goto early_exit;
   }

   if (!ahead_p(shandle))
   {
      retval = ate_error_wrong_type_var(shandle, "ate handle");
      goto early_exit;
   }

   AHEAD *old_head = ahead_cell(shandle);
   AHEAD *new_head = NULL;
   if (ate_create_indexed_head(&new_head, old_head->array, old_head->row_size))
   {
      shandle->value = (char*)new_head;
      free(old_head);
      retval = EXECUTION_SUCCESS;
   }

  early_exit:
   return retval;
}

int ate_action_get_field_sizes(const char *name_handle, const char *name_value,
                               const char *name_array, WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval)
      goto early_exit;

   int row_size = handle->row_size;
   int *col_sizes = (int*)alloca(row_size * sizeof(int));
   memset(col_sizes, 0, row_size * sizeof(int));

   ARRAY_ELEMENT *head = array_head(array_cell(handle->array));
   if (!head)
   {
      retval = ate_error_var_not_found("array_head");
      goto early_exit;
   }

   ARRAY_ELEMENT *ptr = head->next;
   int *size_ptr;
   int counter = 0;
   int curlen;
   while (ptr != head)
   {
      size_ptr = &col_sizes[counter % row_size];
      curlen = strlen(ptr->value);
      if (curlen > *size_ptr)
         *size_ptr = curlen;

      ptr = ptr->next;
      ++counter;
   }

   char numbuffer[32];
   SHELL_VAR *retarr = NULL;
   if (prepare_clean_array_var(&retarr, name_array))
   {
      ARRAY *array = array_cell(retarr);
      for (int i=0; i<row_size; ++i)
      {
         snprintf(numbuffer, sizeof(numbuffer), "%d", col_sizes[i]);
         if (array_insert(array, i, numbuffer))
         {
            retval = ate_error_unexpected();
            break;
         }
      }
   }
   else
      retval = ate_error_unexpected();


  early_exit:
   return retval;
}

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
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval)
      goto early_exit;

   const char *callback_name = NULL;
   SHELL_VAR *callback_func = NULL;
   if (get_string_from_list(&callback_name, extra, 0))
   {
      callback_func = find_function(callback_name);
      if (callback_func == NULL)
      {
         retval = ate_error_function_not_found(callback_name);
         goto early_exit;
      }
   }

   // A name for a new handle variable is required
   const char *name_new_handle = NULL;
   if (!get_string_from_list(&name_new_handle, extra, 1))
   {
      retval = ate_error_missing_arguments("sort");
      goto early_exit;
   }

   AHEAD *head = NULL;
   if (ate_create_indexed_head(&head, handle->array, handle->row_size))
   {
      static const char name_stem[] = "ATE_QSORT_VAR_";
      int buffer_len = strlen(name_stem) + 5;   // '9999\0'
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
         retval = ate_error_failed_to_make_handle("installation of sorted head failed");
      }
   }

  early_exit:
   return retval;
}


/**
 * @brief Initiates a series of invocations of a callback function
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         the extra arguments should start with the
 *                        name of the callback function, possibly
 *                        followed by an optional starting index,
 *                        possibly again followed by the number of
 *                        rows to send.
 *
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_walk_rows(const char *name_handle, const char *name_value,
                         const char *name_array, WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval)
      goto early_exit;

   // No need to continue if there are no rows
   if (handle->row_count == 0)
      goto early_exit;

   const char *func_name = NULL;
   if (!get_string_from_list(&func_name, extra, 0))
   {
      retval = ate_error_missing_usage("callback function");
      goto early_exit;
   }

   int row_count = handle->row_count;
   int row_size = handle->row_size;

   const char *int_str = NULL;
   char *int_end = NULL;

   int starting_index = 0;
   if (get_string_from_list(&int_str, extra, 1))
   {
      starting_index = strtol(int_str, &int_end, 10);
      if (starting_index==0 && int_end == int_str)
      {
         retval = ate_error_arg_not_number(int_str);
         goto early_exit;
      }

      if (starting_index >= row_count)
      {
         retval = ate_error_record_out_of_range(starting_index, row_count);
         goto early_exit;
      }
   }

   int row_limit = row_count;
   if (get_string_from_list(&int_str, extra, 2))
   {
      row_limit = strtol(int_str, &int_end, 10);
      if (row_limit==0 && int_end == int_str)
      {
         retval = ate_error_arg_not_number(int_str);
         goto early_exit;
      }
      // Force reasonable limit
      row_limit += starting_index;
      if (row_limit > row_count)
         row_limit = row_count;
   }

   SHELL_VAR *func_var = find_function(func_name);
   if (!func_var)
   {
      retval = ate_error_function_not_found(func_name);
      goto early_exit;
   }

   SHELL_VAR *array_var = ate_get_prepared_variable(name_array, att_array);;
   ARRAY *array = NULL;
   if (array_var)
   {
      array = array_cell(array_var);
      if (NULL == array)
      {
         array = array_create();
         if (NULL != array)
            array_var->value = (char*)array;
      }
   }

   if (NULL == array)
   {
      retval = ate_error_var_not_found(name_array);
      goto early_exit;
   }

   ARRAY_ELEMENT *current_ael;

   // Uniquely process the callback exitcode,
   // 0 to continue,
   // 1 to stop
   int callback_result;

   for (int i = starting_index; i < row_limit; ++i)
   {
      array_flush(array);
      if (NULL == (current_ael = ate_get_indexed_row(handle, i)))
      {
         retval = ate_error_corrupt_table();
         goto early_exit;
      }

      for (int ei=0; ei < row_size && current_ael; ++ei)
      {
         array_insert(array, ei, current_ael->value);
         current_ael = current_ael->next;
      }

      // CALL CALLBACK HERE
      char *numstr = itos(i);
      callback_result = invoke_shell_function(func_var, numstr, name_array, NULL);
      free(numstr);

      if (callback_result != 0)
         break;
   }

  early_exit:
   return retval;
}
