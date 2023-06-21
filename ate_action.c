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
   {
      fprintf(stderr, "Array parameter missing.\n");
      retval = EX_BADUSAGE;
   }

   int row_size = 1;
   if (get_int_from_list(&row_size, extra, 1) && row_size < 1)
   {
      fprintf(stderr, "Invalid row size of %d was changed to 1.\n", row_size);
      row_size = 1;
   }


   if (retval == EXECUTION_SUCCESS)
   {
      SHELL_VAR *hvar = NULL;
      if (!ate_create_handle(&hvar, name_handle, array, row_size))
         retval = EXECUTION_FAILURE;
   }

   return retval;
}


int ate_action_declare(const char *name_handle,
                       const char *name_value,
                       const char *name_array,
                       WORD_LIST *extra)
{
   int retval;

   


   return retval;
}

ATE_AGENT agent_pool[] = {
   { "declare", ate_action_declare, "Create a new ate object" }
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

   fprintf(stderr, "Action '%s' not found.\n", name_action);

   return EX_NOTFOUND;
}
