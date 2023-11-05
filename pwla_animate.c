/* -*- mode: c -*- */

#include "pwla.h"

#include <stdio.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "word_list_stack.h"

#include "animate_utilities.h"

/**
 * @brief FAKE ACTION animate
 */
int pwla_animate(ARG_LIST *alist)
{
   const char *action_name = "animate";
   const char *handle_name = NULL;
   const char *value_name = NULL;
   const char *array_name = NULL;
   const char *start_row_str = NULL;

   ARG_TARGET animate_targets[] = {
      { "handle_name", AL_ARG, &handle_name},
      { "v",           AL_OPT, &value_name},
      { "a",           AL_OPT, &array_name},
      { "-s",          AL_ARG, &start_row_str},
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(animate_targets, alist, action_name, 0)))
       goto early_exit;

   // Gotta do this (with a couple of exceptions):
   // get the handle variable
   SHELL_VAR *handle_var;
   if ((retval = get_handle_var_by_name_or_fail(&handle_var,
                                                handle_name,
                                                action_name)))
      goto early_exit;

   // The following values are (probably) the user-modifiable
   // parameters for running this action.  I want to design the
   // action with default values for the expected option settings
   // int top_index = 0;
   // int reserve_top = 0;
   // int reserve_bottom = 0;
   // const char *line_print_callback = NULL;

   retval = EX_USAGE;

   int wide, tall;
   get_screen_size(&wide, &tall);

   reset_screen();
   set_cursor_position(1,1);

   APARAMS aparams = { ahead_cell(handle_var), NULL, 0, 0, 2, 2, 4, 4 };
   update_data_boundaries(&aparams);
   replot_data(&aparams);
   erase_data_window(&aparams);

   retval = EXECUTION_SUCCESS;

  early_exit:
   return retval;
}

