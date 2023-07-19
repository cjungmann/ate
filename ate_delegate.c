/**
 * @file ate_delegate.c
 * @brief Code here manages negotions for actions and printing help
 *        pages by use of the ATE_AGENT array of actions.
 */

#include <builtins.h>
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif
#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>
#include <stdlib.h>   // for getenv()

#include "ate_action.h"
#include "ate_errors.h"

ATE_AGENT agent_pool[] = {
   { "list_actions", ate_action_list_actions, "Show available actions",
     "ate list_actions"
   }
   , { "show_action", ate_action_show_action, "Show action usage",
     "ate show_action [action_name]"
   }
   , { "declare", ate_action_declare, "Create a new ate handle",
      "ate declare name_handle [name_array] [row_size]"
   }
   , { "get_row_count", ate_action_get_row_count, "Get count of indexed rows",
      "ate get_row_count name_handle [-v result_variable]"
   }
   , { "get_row_size", ate_action_get_row_size, "Get number of fields in a row",
      "ate get_row_size name_handle [-v result_variable]"
   }
   , { "get_array_name", ate_action_get_array_name, "Get name of hosted array",
      "ate get_array_name handle_name [-v result_variable]"
   }
   , { "get_row", ate_action_get_row, "Get row by index number",
      "ate get_row name_handle row_number [-a result_array]"
   }
   , { "put_row", ate_action_put_row, "Put row by index number",
      "ate put_row name_handle row_number name_row_array"
   }
   , { "append_data",   ate_action_append_data,   "Add data to end of table",
      "ate append_data name_handle [value ...]",
   }
   , { "index_rows",  ate_action_index_rows, "Create new index to row heads",
      "ate index_rows name_handle"
   }
   , { "reindex_elements",  ate_action_reindex_elements,
      "Change array element indexes to match row index",
      "ate reindex_elements name_handle"
   }
   , { "get_field_sizes",  ate_action_get_field_sizes,
      "Get the size of the longest element of each table field.",
      "ate get_field_sizes name_handle [-a result_array]"
   }
   , { "sort", ate_action_sort, "quick-sort rows",
      "ate sort name_handle name_callback name_new_handle"
   }
   , { "resize_rows", ate_action_resize_rows, "set a new row size for the table",
      "ate resize_rows name_handle new_row_size"
   }
   , { "filter", ate_action_filter, "make handle with subset of a table's contents",
      "ate resize_rows name_handle filter_func new_handle_name"
   }
   , { "walk_rows", ate_action_walk_rows, "invoke callback function with each row in table",
      "ate walk_rows name_handle name_callback [-s start_row] [-c count_of_rows]"
   }
};

const int agent_count = sizeof(agent_pool) / sizeof(ATE_AGENT);

/** Used to test for array limit in @ref delegate_action. */
const ATE_AGENT *agent_limit = agent_pool + agent_count;

/**
 * @brief Print a simple list of available actions
 */
void delegate_list_actions(void)
{
   const ATE_AGENT *agent = (const ATE_AGENT*)agent_pool;
   while (agent < agent_limit)
   {
      printf("%s\n", agent->name);
      ++agent;
   }
}

/**
 * @brief Prints the usage and description for each enumerated action.
 * @param "action_name"   name of single requested action.  If this
 *                        argument is NULL, all actions will be
 *                        described.
 */
void delegate_show_action_usage(const char *action_name)
{
   // man termcap(1) for for LESS_TERMCAP_xx meanings:
   const char *bold = getenv("LESS_TERMCAP_md");
   const char *unbold = getenv("LESS_TERMCAP_me");

   if (bold == NULL)
      bold = "\x1b[1m";
   if (unbold == NULL)
      unbold = "\x1b[0m";

   int printed = 0;
   const ATE_AGENT *agent = (const ATE_AGENT*)agent_pool;
   while (agent < agent_limit)
   {
      if (action_name==NULL || strcmp(action_name,agent->name)==0)
      {
         ++printed;

         // Extra line before action if after first action
         if (agent > (const ATE_AGENT*)agent_pool)
            printf("\n");

         printf("%s%s%s  %s\n", bold, agent->name, unbold, agent->description);
         printf("usage:\n  %s\n", agent->usage );
      }

      ++agent;
   }

   if (printed==0 && action_name)
      printf("Requested action (%s) was not found.\n", action_name);
}

/**
 * @brief Assembles resources, finds appropriate agent, then executes
 *        the action.
 *
 * @param "name_action"   string with which to search the agent pool
 * @param "name_handle"   name of SHELL_VAR containint the AHEAD
 * @param "name_value"    name to override default SHELL_VAR return
 *                        variable
 * @param "name_array"    name to override default ARRAY SHELL_VAR to
 *                        return multiple values (or a row)
 * @param "extra"         unaccounted-for command line arguments to
 *                        interpreted as appropriate for the specific
 *                        action
 *
 * @return EXECUTION_SUCCESS (0) or an error value according to the
 *         values found in `include/bash/shell.h`.
 */
int delegate_action(const char *name_action,
                    const char *name_handle,
                    const char *name_value,
                    const char *name_array,
                    WORD_LIST *extra)
{
   ATE_AGENT *agent = agent_pool;
   while (agent < agent_limit)
   {
      if (strcmp(name_action, agent->name)==0)
         return (*agent->action)(name_handle, name_value, name_array, extra);

      ++agent;
   }

   ate_register_error("'%s' is not a recognized action name", name_action);
   return EX_USAGE;
}
