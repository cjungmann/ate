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

#include <builtins/common.h>  // for builtin_error() function

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
static int ate_builtin(WORD_LIST *list)
{
   if (list == NULL)
   {
      builtin_error("action name expected: see 'help ate'.");
      return EXECUTION_FAILURE;
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
         return (*(ptr->func))(alist);
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
   "Treats Bash arrays as fast tables, with database characteristics",
   "",
   "Create an ate table for very fast indexed access.  Read and write",
   "rows.  Sort tables or generate keys for alternative orders and",
   "indexed-sequential access.",
   "",
   "There are two reference actions:",
   "  'ate list_actions' for a list of action names,",
   "  'ate show_action action_name' to show the arguments of the",
   "       'action_name' action.",
   "",
   "  'ate show_action' without an action name shows all actions.",
   "",
   "See ate(1) for usage details or ate(7) for a tutorial.",
   (char*)NULL     // end of array marker
};

struct builtin ate_struct = {
   .name      = "ate",
   .function  = ate_builtin,
   .flags     = BUILTIN_ENABLED,
   .long_doc  = desc_ate,
   .short_doc = "ate action_name [options] [arguments]",
   .handle    = 0
};


