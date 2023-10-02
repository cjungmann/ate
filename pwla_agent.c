#include "pwla.h"
#include "ate_errors.h"

#include <stdio.h>

#include "pwla_agent_list.def"

/**
 * @brief Calculated total of catalog implementation entries.
 */
int pwla_action_count = sizeof(pwla_actions) / sizeof(struct pwla_action_def);


/**
 * @brief Show simple list of action names
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 */
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

/**
 * @brief Show usage for one or all of the catalog entries
 * @param "alist"   Stack-based simple linked list of argument values
 * @return EXECUTION_SUCCESS or one of the failure codes
 */
int pwla_show_action(ARG_LIST *alist)
{
   const char *action_name = "show_action";
   const char *desired_action = NULL;
   ARG_TARGET filter_targets[] = {
      { "action_name",     AL_ARG, &desired_action },
      { NULL }
   };

   int retval;

   if ((retval = process_word_list_args(filter_targets, alist, action_name, 0)))
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

