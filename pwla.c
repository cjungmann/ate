/**
 * @file pwla.c
 * @brief implementation of ate actionss
 *
 * "pwla" stands for __process word-list arguments__
 */

#include "pwla.h"
#include "ate_errors.h"
#include <stdio.h>

/**
 * @brief Called by @ref process_word_list_args to get the matching option target.
 *
 * @param "targets"   Prepared list of arguments
 * @param "option"    Option to be sought
 * @return matching ARG_TARGET if found, NULL otherwise
 */
ARG_TARGET *pwla_find_option_target(ARG_TARGET *targets, char option)
{
   ARG_TARGET *ptr = targets;
   while (ptr->name)
   {
      if (ptr->name[0] == option && (ptr->type == AL_OPT || ptr->type == AL_FLAG))
         return ptr;

      ++ptr;
   }

   return NULL;
}

/**
 * @brief Called by @ref process_word_list_args to get the next unused argument target.
 *
 * @param "targets"   Prepared list of arguments
 * @return AL_ARG target if any remain, otherwise NULL
 */
ARG_TARGET *pwla_next_arg_target(ARG_TARGET *targets)
{
   ARG_TARGET *ptr = targets;
   while (ptr->name)
   {
      if (ptr->type==AL_ARG && *ptr->value == NULL)
         return ptr;

      ++ptr;
   }

   return NULL;
}

/**
 * @brief Match argument targets with arguments to set string variables.
 *
 * This function is a more flexible argument processor that the
 * `getopts` Bash builtin.  It allows for options (indicated by a
 * hyphen-prefixed character) to be entered anywhere in the list of
 * command line arguments.
 *
 * The targets are an array of structs (@ref arg_target) that
 * identify the match and include a string-pointer-pointer to which
 * argument or option's value will be saved.
 *
 * Options are matched as found, and arguments are matched in the
 * order they appear in the target list.  Once a target is matched,
 * the target element is discarded, with the pointer from the target
 * element now pointing to the target following the discarded target.
 *
 * It is assumed that the target list is a stack-based linked-list,
 * most likely to have been created with the @ref args_from_word_list
 * macro in @ref pwla.h.
 *
 * @param "targets"      array of argument targets
 * @param "args_handle"  simple stack-based linked list of command line arguments
 * @param "flags"        for future use, may modify how arguments are discarded
 * @return EXECUTION_SUCCESS if everything went OK, non-zero for any failure.
 */
int process_word_list_args(ARG_TARGET *targets, ARG_LIST *args_handle, AL_FLAGS flags)
{
   ARG_TARGET *target_head = targets;
   ARG_TARGET *cur_target;

   ARG_LIST *arg_handle = args_handle;

   while (arg_handle->next)
   {
      const char *arg_val = arg_handle->next->value;
      if (arg_val[0] == '-')
      {
         char cur_option = arg_val[1];

         cur_target = pwla_find_option_target(target_head, cur_option);

         if (cur_target == NULL)
         {
            // Update the handle without discarding the current argument
            // (save it for follow-on processing):
            arg_handle = arg_handle->next;
            continue;
         }

         // Leave if last chance argument parsing and it's an
         // unrecognized option
         if (cur_target->type == AL_ARG && flags & AL_NOTIFY_UNKNOWN)
         {
            ate_register_unknown_option(cur_option);
            return EX_USAGE;
         }

         // Branch processing based on argument or flag option type
         if (cur_target->type == AL_FLAG)
         {
            // A flag-type value is set with the option argument to indicate a match
            *(cur_target->value) = arg_val;
         }
         else if (cur_target->type == AL_OPT)
         {
            // Copy value to found target:
            // the text following the option letter, if any,
            if (arg_val[2])
               *(cur_target->value) = &arg_val[2];
            // or the text of the following argument (if any),xs
            else if (arg_handle->next->next)
            {
               *(cur_target->value) = arg_handle->next->next->value;
               // discard the next argument since we're consuming two command line args
               arg_handle->next = arg_handle->next->next;
            }

            if (*(cur_target->value) == NULL)
            {
               ate_register_option_missing_argument(cur_option);
               return EX_USAGE;
            }
         }
      }
      else
      {
         cur_target = pwla_next_arg_target(targets);
         if (cur_target == NULL)
         {
            // Update the handle without discarding the current argument
            // (save it for follow-on processing):
            arg_handle = arg_handle->next;
            continue;
         }

         // Copy value to found target
         *cur_target->value = arg_val;
      }

      // discard consumed argument
      arg_handle->next = arg_handle->next->next;
   }

   return EXECUTION_SUCCESS;
}


/**
 * @brief debugging tool to see disposition of targets
 * @param "targets"   list of remaining targets
 * @param "action"    name of action from which the targets are being consumed
 */
void dump_targets(ARG_TARGET *targets, const char *action)
{
   ARG_TARGET *tptr = targets;
   const char *value;
   while (tptr && tptr->name)
   {
      if (tptr->value)
         value = *tptr->value;
      else
         value = "N/A";

      printf("\x1b[32;1mPWLA_DUMP %s\x1b[m target \x1b[32;1m%s\x1b[m is '%s'\n",
             action,
             tptr->name,
             value);

      ++tptr;
   }
}

