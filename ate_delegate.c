#include <builtins.h>
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif
#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

#include <stdio.h>

#include "ate_action.h"
#include "ate_errors.h"

ATE_AGENT agent_pool[] = {
   { "declare",        ate_action_declare,       "Create a new ate object" }
   , {"get_row_count", ate_action_get_row_count, "Get count of indexed rows" }
   , {"get_row",       ate_action_get_row,       "Get row by index number" }
   , {"put_row",       ate_action_put_row,       "Put row by index number" }
   , {"append_data",   ate_action_append_data,   "Add data to end of table" }
   // , {"seek_row"       ate_seek_row"             "Seek row with callback" }
   // , {"seek_row_field" ate_seek_row_field"       "Seek row by field" }
};

const int agent_count = sizeof(agent_pool) / sizeof(ATE_AGENT);

/** Used to test for array limit in @ref delegate_action. */
const ATE_AGENT *agent_limit = agent_pool + agent_count;

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

   return ate_error_action_not_found(name_action);
}
