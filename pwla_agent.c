#include "pwla.h"
#include "ate_errors.h"

#include <stdio.h>


struct pwla_action_def pwla_actions[] = {
   {"list_actions", "show list of available actions",
    "ate list_actions",
    pwla_list_actions },

   {"show_action", "show usage information for a given action",
    "ate show_action",
    pwla_show_action },

   { "declare", "initialize a new handle",
     "ate declare handle_name row_size [array_name]",
     pwla_declare },

   { "append_data", "add data to the table",
     "ate append_data handle_name [values ...]",
     pwla_append_data },

   { "index_rows", "update index to table rows (after append_data)",
     "ate index_rows handle_name",
     pwla_index_rows },

   { "get_row_count", "get number of rows in the virtual table",
     "ate get_row_count handle_name [-v result_name]",
     pwla_get_row_count },

   { "get_row_size", "get number or elements in a virtual row",
     "ate get_row_size handle_name [-v result_name]",
     pwla_get_row_size },

   { "get_array_name", "get name of the hosted table array",
     "ate get_array_name handle_name [-v result_name]",
     pwla_get_array_name },

   { "get_field_sizes", "get table's max field sizes to an array",
     "ate get_field_sizes handle_name [-a result_array_name]",
     pwla_get_field_sizes },

   { "get_row", "get specified row's contents to an array",
     "ate get_row handle_name row_number [-a result_array_name]",
     pwla_get_row },

   { "put_row", "update a specified table row with an array's contents",
     "ate put_row handle_name row_number array_name",
     pwla_put_row },

   { "resize_rows", "change the number of elements in a virtual row",
     "ate resize_rows handle_name new_row_size",
     pwla_resize_rows },

   { "reindex_elements", "update the hosted array's contents to match index",
     "ate reindex_elements handle_name",
     pwla_reindex_elements },

   { "walk_rows", "invoke a callback with each of a table's rows",
     "ate walk_rows handle_name function_name [-s start] [-c count] [extra ...]",
     pwla_walk_rows },

   { "sort", "create a duplicate handle with a sorted order",
     "ate sort handle_name comparison_function [extra ...]",
     pwla_sort },

   { "filter", "create a duplicate handle with filtered contents",
     "ate filter handle_name filter_function [extra ...]",
     pwla_filter }
};

int pwla_action_count = sizeof(pwla_actions) / sizeof(struct pwla_action_def);

int pwla_list_actions(ARG_LIST *alist)
{
   struct pwla_action_def *ptr = pwla_actions;
   struct pwla_action_def *end = ptr + pwla_action_count;

   while (ptr < end)
   {
      printf("%s\n", ptr->name);
      ++ptr;
   }

   printf("\n");

   return EXECUTION_SUCCESS;
}

int pwla_show_action(ARG_LIST *alist)
{
   const char *desired_action = NULL;
   ARG_TARGET filter_targets[] = {
      { "action_name",     AL_ARG, &desired_action },
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(filter_targets, alist, 0)))
       goto early_exit;

   struct pwla_action_def *ptr = pwla_actions;
   struct pwla_action_def *end = ptr + pwla_action_count;

   while (ptr < end)
   {
      if (!desired_action || 0 == strcmp(ptr->name, desired_action))
      {
         printf("\n%s: %s\n", ptr->name, ptr->desc);
         printf("  %s\n", ptr->usage);
      }
      ++ptr;
   }

   printf("\n");

  early_exit:
   return retval;
}

