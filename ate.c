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

#include <stdio.h>

#include "word_list_stack.h"
#include "ate_errors.h"

#include "pwla.h"

const char Error_Name[] = "ATE_ERROR";


// Found in pwla_agent.c
extern struct pwla_action_def pwla_actions[];
extern int pwla_action_count;

/**
 * @brief effective `main` function, initial entry of an instance
 *        of `ate`.
 *
 * @param "list"   list of command line arguments
 * @return EXECUTION_SUCCESS or an error-dignalling non-zero value.
 */
static int ate(WORD_LIST *list)
{
   if (list == NULL)
   {
      ate_register_error("no action name provide for ate");
      return EX_USAGE;
   }

   const char *action = list->word->word;

   struct pwla_action_def *ptr = pwla_actions;
   struct pwla_action_def *end = ptr + pwla_action_count;

   while (ptr < end)
   {
      if (0 == strcmp(action, ptr->name))
      {
         ARG_LIST *alist = NULL;
         args_from_word_list(alist, list->next);
         int retval = (*(ptr->func))(alist);
         // if (retval == EX_USAGE)
         // {
         //    builtin_usage();
         //    // printf("usage:\n  %s\n", ptr->usage);
         // }

         return retval;
      }

      ++ptr;
   }

   ate_register_error("'%s' is not a recognized action at this time.", action);
   return EX_USAGE;
}

// static int ate(WORD_LIST *list)
// {
//    return pwla_run(list);
// }

static char *desc_ate[] = {
   "ate - Table extension for Bash arrays",
   "",
   "ate list_actions                quick list of action names",
   "ate show_action [action_name]   details about action_name",
   "",
   "Declare an Array Table Extension handle, then use one of several",
   "actions to interact with the table.",
   "",
   "All 'ate' actions except 'declare', 'list_actions, and",
   "'show_action' require an initialized ate handle.  The",
   "'declare' action creates an ate handle (see 'man ate(1)').",
   "",
   "When ate returns information, the results are written to shell",
   "variables that can be read in Bash.  Single value results",
   "like 'row_size' or 'row_count' are written to Bash variable",
   "ATE_VALUE by default.  Table row results are written to Bash",
   "variable ATE_ARRAY.",
   "",
   "The result variables can be changed using '-v' and '-a' options:",
   "",
   "  -a array    Use _array_ as the name of array to be used for",
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


