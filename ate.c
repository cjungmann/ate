/**
 * @file ate.c
 * @brief Effective `main` function `ate` to be exposed as a
 *        loadable Bash builtin
 */

/**
 * @mainpage
 *
 * Developers' Documentation for the **ate** Loadable Bash Builtin
 *
 * This doxygen-generated documentation is intended to discuss code
 * design and objectives for developers who might want to make changes
 * to the **ate** builtin.
 *
 * Documentation about the use of **ate** is contained in a **man**
 * page (**ate(1)**).  The **man** page is for Bash script developers
 * who want to use this utility.
 *
 * I have made functions' documentation available under the
 * **Modules** main menu item by enclosing the function prototyes in
 * @@defgroup structures.  It is a workaround against Doxygen's lame
 * lack of direct access to function documentation.
 */

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>

#include "word_list_stack.h"
#include "ate_action.h"
#include "ate_errors.h"

/**
 * @brief effective `main` function, initial entry of an instance
 *        of `ate`.
 *
 * @param "list"   list of command line arguments
 * @return EXECUTION_SUCCESS or an error message.
 */
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
      else if (name_action == NULL)
         name_action = cur_arg;
      else if (name_handle == NULL)
         name_handle = cur_arg;
      else
      {
         WL_APPEND(etail, cur_arg);
         if (!extras)
            extras = etail;
      }

      ptr = ptr->next;
   }

   if (name_action == NULL)
   {
      retval = ate_error_missing_action();
      builtin_usage();
      goto exit_for_error;
   }

   retval = delegate_action(name_action,
                            name_handle,
                            name_result_value,
                            name_result_array,
                            extras);

  exit_for_error:
   return retval;
}

static char *desc_ate[] = {
   "Use the 'ate' command to create and manipulate a table-like",
   "view of a Bash array.",
   "",
   "Use one of several 'action_name' values to interact with the",
   "table (see 'ate list_actions` or `ate show_action`).",
   "",
   "All 'ate' actions except 'declare', 'list_actions, and",
   "'show_action' require an initialized ate handle.  The",
   "'declare' action creates an ate handle (see 'man ate(1)').",
   "",
   "When ate returns information, the results are written to shell",
   "variables that are accessible in Bash.  Single value results",
   "like 'row_size' or 'row_count' are written to Bash variable",
   "ATE_VALUE by default.  Table row results are written to Bash",
   "variable ATE_ARRAY.",
   "",
   "The result variables can be changed using '-v' and '-a' options:",
   "",
   "  -a  array   Use _array_ as the name of array to be used for",
   "              returning multi-value results when appropriate",
   "              for the action type.",
   "  -v value    Use _value_ as the name of the shell variable to",
   "              be used for return single value results when",
   "              appropriate for the action type.",
   "",
   "Some ate actions take additional arguments that follow the",
   "'action_name', 'handle_name' arguments and any options.  Look",
   "at man ate(1) or 'ate show_action' for more details.",
   (char*)NULL     // end of array marker
};

struct builtin ate_struct = {
   .name      = "ate",
   .function  = ate,
   .flags     = BUILTIN_ENABLED,
   .long_doc  = desc_ate,
   .short_doc = "ate action_name [handle_name] [-a array_name] [-v value_name] [value ...]",
   .handle    = 0
};


