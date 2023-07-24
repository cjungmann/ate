#include "pwla.h"

ARG_TARGET *pwla_find_option_target(ARG_TARGET *targets, char option)
{
   ARG_TARGET *ptr = targets;
   while (ptr->name)
   {
      if (ptr->type == AL_OPT && ptr->name[0] == option)
         return ptr;

      ++ptr;
   }

   return NULL;
}

ARG_TARGET *pwla_next_arg_target(ARG_TARGET *targets)
{
   ARG_TARGET *ptr = targets;
   while (ptr->name)
   {
      if (ptr->type==AL_ARG && *ptr->value == NULL)
         return ptr;

      ++ptr;
   }

   return NULL;
}

int process_word_list_args(ARG_TARGET *targets, ARG_LIST *args_handle, AL_FLAGS flags)
{
   ARG_TARGET *target_head = targets;
   ARG_TARGET *cur_target;

   ARG_LIST *arg_handle = args_handle;

   while (arg_handle->next)
   {
      const char *arg_val = arg_handle->next->value;
      if (arg_val[0] == '-')
      {
         cur_target = pwla_find_option_target(target_head, arg_val[1]);

         if (cur_target == NULL)
         {
            // Update the handle without discarding the current argument
            // (save it for follow-on processing):
            arg_handle = arg_handle->next;
            continue;
         }

         // Copy value to found target:
         // the text following the option letter, if any,
         if (arg_val[2])
            *(cur_target->value) = &arg_val[2];
         // or the text of the following argument (if any),xs
         else if (arg_handle->next->next)
         {
            *(cur_target->value) = arg_handle->next->next->value;
            // discard the next argument since we're consuming two command line args
            arg_handle->next = arg_handle->next->next;
         }
      }
      else
      {
         cur_target = pwla_next_arg_target(targets);
         if (cur_target == NULL)
         {
            // Update the handle without discarding the current argument
            // (save it for follow-on processing):
            arg_handle = arg_handle->next;
            continue;
         }

         // Copy value to found target
         *cur_target->value = arg_val;

         if (cur_target->shellvar != NULL)
            *cur_target->shellvar = find_variable(arg_val);
      }

      // discard consumed argument
      arg_handle->next = arg_handle->next->next;
   }

   return EXECUTION_SUCCESS;
}

/*******************
 *     Testing     *
 *******************/

#include "word_list_stack.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

void dump_targets(ARG_TARGET *targets, const char *action)
{
   ARG_TARGET *tptr = targets;
   const char *value;
   while (tptr && tptr->name)
   {
      if (tptr->value)
         value = *tptr->value;
      else
         value = "N/A";

      printf("\x1b[32;1mPWLA_DUMP %s\x1b[m target \x1b[32;1m%s\x1b[m is '%s'\n",
             action,
             tptr->name,
             value);

      ++tptr;
   }
}

/**
 * @brief FAKE ACTION template
 */
int pwla_template(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *value_name = NULL;
   const char *array_name = NULL;

   ARG_TARGET template_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "v",           AL_OPT, &value_name},
      { "a",           AL_OPT, &array_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(template_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "template")))
      goto early_exit;

   SHELL_VAR *value_var;
   if ((retval = create_var_by_given_or_default_name(&value_var,
                                                     value_name,
                                                     DEFAULT_VALUE_NAME,
                                                     "template")))
      goto early_exit;

   SHELL_VAR *array_var;
   if ((retval = create_array_var_by_given_or_default_name(&array_var,
                                                           array_name,
                                                           DEFAULT_ARRAY_NAME,
                                                           "template")))
      goto early_exit;

   if (value_var)
      retval = set_var_from_int(value_var, (ahead_cell(handle_var))->row_count);

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION: declare
 */
int pwla_declare(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *row_size_name = NULL;
   const char *array_name = NULL;

   ARG_TARGET declare_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "row_size",    AL_ARG, &row_size_name},
      { "array_name",  AL_ARG, &array_name},
      { NULL }
   };

   int retval = process_word_list_args(declare_targets, alist, 0);
   // dump_targets(declare_targets, "declare");

   if (retval != EXECUTION_SUCCESS)
      goto early_exit;

   // Most common error is usage, make it the default
   retval = EX_USAGE;

   /*** get and validate required row_size value ***/
   int row_size = 0;
   if (row_size_name)
      get_int_from_string(&row_size, row_size_name);

   if (row_size < 1)
   {
      ate_register_error("invalid or missing row_size value");
      goto early_exit;
   }

   /*** get or make hosted array ***/

   SHELL_VAR *array_var = NULL;  // array for use to be left intact when function ends
   SHELL_VAR *new_array = NULL;  // disposable array in case no array_name and later failure

   if (array_name)
   {
      retval = get_array_var_by_name_or_fail(&array_var, array_name, "declare");
      if (retval)
         goto early_exit;

      int el_count = array_num_elements(array_cell(array_var));
      if (el_count % row_size)
      {
         ate_register_invalid_row_size(row_size, el_count);
         goto early_exit;
      }
   }
   else
   {
      retval = create_array_var_by_stem(&new_array, "ATE_HOSTED_ARRAY_", "declare");
      if (retval)
         goto early_exit;
      else
         array_var = new_array;
   }

   SHELL_VAR *sv = NULL;
   if (ate_create_handle(&sv, handle_name, array_var, row_size))
      retval = EXECUTION_SUCCESS;
   else
   {
      retval = EXECUTION_FAILURE;
      ate_register_error("failed to create handle in action 'declare'");
      if (new_array)
         dispose_variable(new_array);
   }

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION append_data
 */
int pwla_append_data(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   ARG_TARGET append_data_targets[] = {
      { "handle_name", AL_ARG, &handle_name },
      { NULL }
   };

   int retval = process_word_list_args(append_data_targets, alist, 0);
   if (retval)
      goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "append_data")))
      goto early_exit;

   AHEAD *ahead = ahead_cell(handle_var);
   ARRAY *array = array_cell(ahead->array);
   int row_size = ahead->row_size;
   const char **values = (const char**)alloca(row_size * sizeof(char*));

   const char **cptr = values;
   const char **cend = cptr + row_size;

   int index = array->max_index;

   ARG_LIST *ptr = alist->next;
   while (ptr)
   {
      // Accumulate strings until a full row's worth
      *(cptr++) = ptr->value;
      if (cptr >= cend)
      {
         // Append the row to the array
         cptr = values;
         while (cptr < cend)
            array_insert(array, ++index, (char*)*(cptr++));

         // reset row pointer to start
         cptr = values;
      }
      ptr = ptr->next;
   }

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION index_rows
 */
int pwla_index_rows(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   ARG_TARGET index_rows_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(index_rows_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "index_rows")))
      goto early_exit;

   // Do the job
   AHEAD *old_head = ahead_cell(handle_var);
   AHEAD *new_head = NULL;
   if (ate_create_indexed_head(&new_head, old_head->array, old_head->row_size))
   {
      handle_var->value = (char*)new_head;
      free(old_head);
      retval = EXECUTION_SUCCESS;
   }
   else
   {
      ate_register_error("unexpected error in action 'index_rows'");
      retval = EXECUTION_FAILURE;
   }


  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION get_row_count
 */
int pwla_get_row_count(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *value_name = NULL;

   ARG_TARGET get_row_count_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "v",           AL_OPT, &value_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(get_row_count_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "get_row_count")))
      goto early_exit;

   SHELL_VAR *value_var;
   if ((retval = create_var_by_given_or_default_name(&value_var,
                                                     value_name,
                                                     DEFAULT_VALUE_NAME,
                                                     "get_row_count")))
      goto early_exit;

   // The arguments are secured, execute the action:
   retval = set_var_from_int(value_var, (ahead_cell(handle_var))->row_count);

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION get_row_size
 */
int pwla_get_row_size(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *value_name = NULL;

   ARG_TARGET template_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "v",           AL_OPT, &value_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(template_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "template")))
      goto early_exit;

   SHELL_VAR *value_var;
   if ((retval = create_var_by_given_or_default_name(&value_var,
                                                     value_name,
                                                     DEFAULT_VALUE_NAME,
                                                     "template")))
      goto early_exit;

   // The arguments are secured, execute the action:
   retval = set_var_from_int(value_var, (ahead_cell(handle_var))->row_size);

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION get_array_name
 */
int pwla_get_array_name(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *value_name = NULL;

   ARG_TARGET template_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "v",           AL_OPT, &value_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(template_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "template")))
      goto early_exit;

   SHELL_VAR *value_var;
   if ((retval = create_var_by_given_or_default_name(&value_var,
                                                     value_name,
                                                     DEFAULT_VALUE_NAME,
                                                     "template")))
      goto early_exit;

   // The arguments are secured, execute the action:
   ate_dispose_variable_value(value_var);
   value_var->value = savestring((ahead_cell(handle_var))->array->name);
   retval = EXECUTION_SUCCESS;

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION get_field_sizes
 */
int pwla_get_field_sizes(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *array_name = NULL;

   ARG_TARGET get_field_sizes_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "a",           AL_OPT, &array_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(get_field_sizes_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "get_field_sizes")))
      goto early_exit;

   SHELL_VAR *array_var;
   if ((retval = create_array_var_by_given_or_default_name(&array_var,
                                                           array_name,
                                                           DEFAULT_ARRAY_NAME,
                                                           "get_field_sizes")))
      goto early_exit;

   // Begin work:

   AHEAD *ahead = ahead_cell(handle_var);

   // Make and zero max-size accumulator array:
   int row_size = ahead->row_size;
   int *row_array = (int*)alloca(row_size * sizeof(int));
   memset(row_array, 0, row_size * sizeof(int));

   // Accumulate largest string length for each column
   ARRAY_ELEMENT **row_ptr = ahead->rows;
   ARRAY_ELEMENT **rows_end = row_ptr + ahead->row_count;
   while (row_ptr < rows_end)
   {
      ARRAY_ELEMENT *el_ptr = *row_ptr;
      int *int_ptr = row_array;
      for (int i=0; i<row_size; ++i)
      {
         int curlen = strlen(el_ptr->value);
         if (curlen > *int_ptr)
            *int_ptr = curlen;

         el_ptr = el_ptr->next;
         ++int_ptr;
      }

      ++row_ptr;
   }

   // Copy accumulated column sizes to return array
   ARRAY *array = array_cell(array_var);
   array_flush(array);
   for (int i=0; i<row_size; ++i)
      array_insert(array, i, itos(row_array[i]));

   retval = EXECUTION_SUCCESS;

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION get_row
 */
int pwla_get_row(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *array_name = NULL;
   const char *row_index_str = NULL;

   ARG_TARGET get_row_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "a",           AL_OPT, &array_name},
      { "row_index  ", AL_ARG, &row_index_str},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(get_row_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "get_row")))
      goto early_exit;

   SHELL_VAR *array_var;
   if ((retval = create_array_var_by_given_or_default_name(&array_var,
                                                           array_name,
                                                           DEFAULT_ARRAY_NAME,
                                                           "get_row")))
      goto early_exit;

   retval = EX_USAGE;
   AHEAD *ahead = ahead_cell(handle_var);
   int row_index = -1;
   if (row_index_str && get_int_from_string(&row_index, row_index_str))
   {
      if (row_index < 0 || row_index >= ahead->row_count)
      {
         ate_register_error("out-of-range row index value (%d out of %d)",
                            row_index, ahead->row_count);
         goto early_exit;
      }
   }
   else
   {
      ate_register_error("missing or bad row index value in 'get_row'");
      goto early_exit;
   }

   ARRAY_ELEMENT *source_el = ahead->rows[row_index];
   ARRAY *target_array = array_cell(array_var);
   array_flush(target_array);
   int ndx;
   for (ndx = 0; source_el && ndx < ahead->row_size; ++ndx)
   {
      array_insert(target_array, ndx, source_el->value);
      source_el = source_el->next;
   }

   if (ndx == ahead->row_size)
      retval = EXECUTION_SUCCESS;
   else
   {
      ate_register_error("incomplete row read in 'get_row'");
      retval = EXECUTION_FAILURE;
   }

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION put_row
 */
int pwla_put_row(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *row_index_str = NULL;
   const char *row_name = NULL;

   ARG_TARGET put_row_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "row_index",   AL_ARG, &row_index_str},
      { "row_name",    AL_ARG, &row_name},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(put_row_targets, alist, 0)))
       goto early_exit;

   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                "put_row")))
      goto early_exit;

   // Need ahead of time to validate values
   AHEAD *ahead = ahead_cell(handle_var);

   SHELL_VAR *array_var = NULL;
   if ((retval = get_array_var_by_name_or_fail(&array_var, row_name, "put_row")))
      goto early_exit;

   retval = EX_USAGE;

   ARRAY *source_array = array_cell(array_var);

   // Confirm appropriate source size
   int source_num_elements = array_num_elements(source_array);
   if (source_num_elements != ahead->row_size)
   {
      ate_register_error("source row size of %d doesn't match table row size of %d",
                         source_num_elements, ahead->row_size);
      goto early_exit;
   }

   // Validate requested row index
   int row_index = -1;
   if (row_index_str && get_int_from_string(&row_index, row_index_str))
   {
      if (row_index < 0 || row_index >= ahead->row_count)
      {
         ate_register_error("out-of-range row index value (%d out of %d)",
                            row_index, ahead->row_count);
         goto early_exit;
      }
   }
   else
   {
      ate_register_error("missing or bad row index value in 'get_row'");
      goto early_exit;
   }

   // Start copying
   ARRAY_ELEMENT *source_el = source_array->head->next;
   ARRAY_ELEMENT *target_el = ahead->rows[row_index];
   for (int ndx = 0; ndx < ahead->row_size; ++ndx)
   {
      free(target_el->value);
      target_el->value = savestring(source_el->value);

      target_el = target_el->next;
      source_el = source_el->next;
   }

   retval = EXECUTION_SUCCESS;

  early_exit:
   return retval;
}

/**
 * @brief FAKE ACTION sort
 */
int pwla_sort(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *compare_function = NULL;
   const char *new_handle_name = NULL;

   ARG_TARGET sort_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "compare_function",  AL_ARG, &compare_function},
      { "handle_name",       AL_ARG, &new_handle_name},
      { NULL }
   };

   int retval = process_word_list_args(sort_targets, alist, 0);
   dump_targets(sort_targets, "sort");

   return retval;
}

/**
 * @brief FAKE ACTION filter
 */
int pwla_filter(ARG_LIST *alist)
{
   const char *handle_name = NULL;
   const char *filter_function = NULL;
   const char *new_handle_name = NULL;

   ARG_TARGET filter_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "filter_function", AL_ARG, &filter_function},
      { "new_handle",      AL_ARG, &new_handle_name},
      { NULL }
   };

   int retval = process_word_list_args(filter_targets, alist, 0);
   dump_targets(filter_targets, "filter");

   return retval;
}

/**
 * @brief FAKE ENTRY like "act()"
 */
int test_pwla(WORD_LIST *wl)
{
   ARG_LIST *alist = NULL;
   args_from_word_list(alist, wl);

   const char *action_name = NULL;

   ARG_TARGET main_targets[] = {
      { "action_name", AL_ARG, &action_name},
      { NULL }
   };

   // WORD_LIST *wl = NULL , *wl_tail = NULL;
   // const char* extra_words[] = { "-c", "10", "bogus", "genuine", NULL };
   // const char **wordarray = extra_words;
   // WL_WALK(wl, wl_tail, wordarray);
   // ARG_LIST *alist = NULL;
   // args_from_word_list(alist, wl);

   int retval = process_word_list_args(main_targets, alist, 0);

   if (!action_name)
   {
      printf("Missing action name\n");
      retval = EX_USAGE;
      goto early_exit;
   }

   if (retval == EXECUTION_SUCCESS && action_name)
   {
      if (0==strcmp(action_name, "declare"))
         retval = pwla_declare(alist);

      // This collection needs only the first four targets to run:
      else if (0==strcmp(action_name, "append_data"))
         retval = pwla_append_data(alist);
      else if (0==strcmp(action_name, "index_rows"))
         retval = pwla_index_rows(alist);
      else if (0==strcmp(action_name, "get_row_count"))
         retval = pwla_get_row_count(alist);
      else if (0==strcmp(action_name, "get_row_size"))
         retval = pwla_get_row_size(alist);
      else if (0==strcmp(action_name, "get_array_name"))
         retval = pwla_get_array_name(alist);
      else if (0==strcmp(action_name, "get_field_sizes"))
         retval = pwla_get_field_sizes(alist);

      // Beginning of actions with supplimental arguments
      else if (0==strcmp(action_name, "get_row"))
         retval = pwla_get_row(alist);
      else if (0==strcmp(action_name, "put_row"))
         retval = pwla_put_row(alist);

      // Beginning of actions invoking callback functions
      // Beginning of actions invoking callback functions
      else if (0==strcmp(action_name, "walk_rows"))
         retval = pwla_walk_rows(alist);
      else if (0==strcmp(action_name, "sort"))
         retval = pwla_sort(alist);
      else if (0==strcmp(action_name, "filter"))
         retval = pwla_filter(alist);

      else
         printf("\x1b[32;1mPWLA\x1b[m No further action available for \x1b[31;1m%s\x1b[m.\n", action_name);
   }

  early_exit:
   // printf("Press ENTER to continue.\n");
   // getchar();

   return retval;
}
