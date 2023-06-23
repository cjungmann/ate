#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>

#include "ate_action.h"
#include "ate_utilities.h"
#include "ate_errors.h"



/**
 * @brief Create a new ate handle.
 * @param "name_handle"   name of handle to create
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         should include one or two values:
 *                        - name of the variable to be put under `ate`
 *                          control
 *                        - optional row size value (integer).  The
 *                          row size value must be at least one, and must
 *                          divide evenly into the number of elements in
 *                          the array.
 * @return EXECUTION_SUCCESS or an error code upon failure
 *        (refer to `include/bash/shell.h`).
 */
int ate_action_declare(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   int retval;

   // Collect additional parameters from _extra_
   const char *name_hosted_array = NULL;
   SHELL_VAR *array = NULL;
   if (get_string_from_list(&name_hosted_array, extra, 0))
   {
      retval = get_shell_var_by_name_and_type(&array,
                                              name_hosted_array,
                                              att_array);
   }
   else
      retval = ate_error_missing_usage("array parameter");

   int row_size = 1;
   if (get_int_from_list(&row_size, extra, 1) && row_size < 1)
      retval = ate_error_invalid_row_size(row_size);

   if (retval == EXECUTION_SUCCESS)
   {
      SHELL_VAR *hvar = NULL;
      if (!ate_create_handle(&hvar, name_handle, array, row_size))
         retval = EXECUTION_FAILURE;
   }

   return retval;
}


/**
 * @brief Returns number of indexed rows.
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    variable name override, default `ATE_VALUE`
 * @param "name_array"    ignored
 * @param "extra"         ignored
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
      {
         ate_dispose_variable_value(var);
         var_setvalue(var, itos(handle->row_count));
         retval = EXECUTION_SUCCESS;
      }
      else
         retval = EX_NOTFOUND;
   }

   return retval;
}

/**
 * @brief Returns an array containing contents of referenced row.
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    array name override, default `ATE_ARRAY`
 * @param "extra"         extra[0] is requested row index (0-based)
 */
int ate_action_get_row(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval == EXECUTION_SUCCESS)
   {
      int requested_index;

      if (get_int_from_list(&requested_index, extra, 0))
      {
         ARRAY_ELEMENT *arel = ate_get_indexed_row(handle, requested_index);
         if (arel)
         {
            ARRAY *arr = array_create();
            if (arr)
            {
               // declared outside of `for` statement to preserve its value
               int i;
               // The row contents in the following elements
               for (i=0; i<handle->row_size && arel; ++i)
               {
                  array_insert(arr, i, arel->value);
                  arel = arel->next;
               }

               // Abort if incomplete row
               if (i < handle->row_size)
               {
                  array_dispose(arr);
                  retval = ate_error_corrupt_table();
               }
               else
               {
                  SHELL_VAR *return_array = NULL;
                  if (get_var_from_list(&return_array, extra, 1))
                  {
                     ate_dispose_variable_value(return_array);
                     return_array->attributes = att_array;
                     return_array->value = (char*)arr;
                  }
                  else
                     retval = ate_error_missing_usage("return row array name");
               }
            }
            else
               retval = ate_error_failed_to_create("an array");
         }
         else
            retval = ate_error_record_out_of_range(requested_index, handle->row_count);
      }
      else
         retval = ate_error_missing_usage("row index in get_row");
   }

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
 */
int ate_action_put_row(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   AHEAD *handle;
   int retval = get_handle_from_name(&handle, name_handle);
   if (retval == EXECUTION_SUCCESS)
   {
      int requested_index;

      if (get_int_from_list(&requested_index, extra, 0))
      {
         ARRAY_ELEMENT *arel_target = ate_get_indexed_row(handle, requested_index);
         if (arel_target)
         {
            const char *array_name = NULL;
            if (get_string_from_list(&array_name, extra, 1))
            {
               SHELL_VAR *avar;
               if ((avar = find_variable(array_name)))
               {
                  if (array_p(avar))
                  {
                     ARRAY_ELEMENT *arel_target_head = ate_get_array_head(handle);
                     ARRAY_ELEMENT *arel_target_ptr = arel_target;
                     int count = 0;

                     ARRAY *arr = array_cell(avar);
                     ARRAY_ELEMENT *arel_source_head = arr->head;
                     ARRAY_ELEMENT *arel_source_ptr = arel_source_head->next;
                     while (count < handle->row_size)
                     {
                        if (arel_source_ptr != arel_source_head &&
                            arel_target_ptr != arel_target_head)
                        {
                           free(arel_target_ptr->value);
                           arel_target_ptr->value = savestring(arel_source_ptr->value);

                           arel_target_ptr = arel_target_ptr->next;
                           arel_source_ptr = arel_source_ptr->next;
                        }
                        else
                           break;
                        ++count;
                     }
                  }
                  else
                     retval = ate_error_wrong_type_var(avar, "array");
               }
               else
                  retval = ate_error_var_not_found(array_name);
            }
            else
               retval = ate_error_missing_usage("array name");
         }
         else
            retval = ate_error_record_out_of_range(requested_index, handle->row_count);
      }
      else
         retval = ate_error_missing_usage("row index in putt_row");
   }

   return retval;
}

/**
 * @brief Updates a table row with the contents of an array
 * @param "name_handle"   name of the reference handle
 * @param "name_value"    ignored
 * @param "name_array"    ignored
 * @param "extra"         list of elements
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


