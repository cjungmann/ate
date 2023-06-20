// -*- mode:c -*-

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>

#include "word_list_stack.h"
#include "ate_handle.h"

static int ate(WORD_LIST *list)
{
   int retval = EXECUTION_SUCCESS;

   // String pointers that can be set with options
   const char *name_handle = NULL;
   const char *name_action = NULL;
   const char *name_result_value = "ATE_VALUE";
   const char *name_result_array = "ATE_ARRAY";
   WORD_LIST *extras = NULL, *etail = NULL;

   // Option parsing status variables
   const char *cur_arg;
   const char **pending_option = NULL;

   WORD_LIST *ptr = list;
   while (ptr)
   {
      cur_arg = ptr->word->word;
      if (pending_option)
      {
         *pending_option = cur_arg;
         pending_option = NULL;
      }
      else if (*cur_arg == '-' && cur_arg[1])
      {
         switch(cur_arg[1])
         {
            case 'a':
               // Immediate resolution when no space between option and value:
               if (cur_arg[2])
                  name_result_array = &cur_arg[2];
               // Defer getting option value until next loop:
               else
                  pending_option = &name_result_array;
               break;

            case 'v':
               if (cur_arg[2])
                  name_result_value = &cur_arg[2];
               else
                  pending_option = &name_result_value;
               break;
         }
      }
      // Positional options
      else if (name_handle == NULL)
         name_handle = cur_arg;
      else if (name_action == NULL)
         name_action = cur_arg;
      else
      {
         WL_APPEND(etail, cur_arg);
         if (!extras)
            extras = etail;
      }

      ptr = ptr->next;
   }

   if (name_handle == NULL || name_action == NULL)
   {
      fprintf(stderr, "Handle and action values are required.\n");
      builtin_usage();
      retval = EX_USAGE;
      goto exit_for_error;
   }

   // Display results of collected command line arguments:
   printf("Variable name_handle is set to '%s'\n", name_handle);
   printf("Variable name_action is set to '%s'\n", name_action);
   printf("Variable name_result_value is set to '%s'\n", name_result_value);
   printf("Variable name_result_array is set to '%s'\n", name_result_array);
   WORD_LIST *wptr = extras;
   int extra_counter = 0;
   while (wptr)
   {
      printf("Extra argument #%d: '%s'.\n", ++extra_counter, wptr->word->word);
      wptr = wptr->next;
   }

   if (name_result_array)
   {
      SHELL_VAR *var = find_variable(name_result_array);
      if (array_p(var))
      {
         AHEAD *head = NULL;
         if (ate_create_indexed_handle(&head, var, 3))
         {
            xfree(head);
         }
      }
   }

  exit_for_error:
   return retval;
}

static char *desc_ate[] = {
   "ate is Array Table Extension",
   "",
   "Options:",
   "  -a  array   Use _array_ as the name of array to be used for",
   "              returning multi-value results when appropriate",
   "              for the action type.",
   "  -v value    Use _value_ as the name of the shell variable to",
   "              be used for return single value results when",
   "              appropriate for the action type.",
   (char*)NULL     // end of array marker
};

struct builtin ate_struct = {
   .name      = "ate",
   .function  = ate,
   .flags     = BUILTIN_ENABLED,
   .long_doc  = desc_ate,
   .short_doc = "ate [-a array_name] [-v value_name] handle_name action_name [...]",
   .handle    = 0
};
