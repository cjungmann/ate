#ifndef ATE_ACTION_H
#define ATE_ACTION_H

#include <variables.h>
#include <array.h>

#include "ate_handle.h"

/**
 * @defgroup ATE_Actions Collection of table action functions.
 *
 * All of these functions conform to the @ref ATE_ACTION typedef,
 * and this type-equivalence makes it possible for them to be part
 * of an array of action information elements that includes a
 * function pointer that can be used to invoke a specific action.
 *
 * With 3 exceptions, all actions require the name of an existing
 * SHELL_VAR hosting an initialized @ref AHEAD structure.  The
 * exceptions are two help-related actions, @ref ate_action_list_actions
 * and @ref ate_action_show_action, and the initializer action,
 * @ref ate_action_declare.
 *
 * The **man** page has complete information about the usage of
 * these actions with example code fragments.
 * @{
 */

/**
 * @brief Minimum viable action function, with allowance for
 *        additional action-specific parameters made by the
 *        @p extra parameter.
 */
typedef int (*ATE_ACTION)(const char *name_handle,
                          const char *name_value,
                          const char *name_array,
                          WORD_LIST *extra);


int ate_action_list_actions(const char *name_handle, const char *name_value,
                            const char *name_array, WORD_LIST *extra);

int ate_action_show_action(const char *name_handle, const char *name_value,
                           const char *name_array, WORD_LIST *extra);

int ate_action_declare(const char *name_handle, const char *name_value,
                       const char *name_array, WORD_LIST *extra);

int ate_action_get_row_count(const char *name_handle, const char *name_value,
                             const char *name_array, WORD_LIST *extra);

int ate_action_get_row_size(const char *name_handle, const char *name_value,
                            const char *name_array, WORD_LIST *extra);

int ate_action_get_array_name(const char *name_handle, const char *name_value,
                              const char *name_array, WORD_LIST *extra);

int ate_action_get_row(const char *name_handle, const char *name_value,
                       const char *name_array, WORD_LIST *extra);

int ate_action_put_row(const char *name_handle, const char *name_value,
                       const char *name_array, WORD_LIST *extra);

int ate_action_append_data(const char *name_handle, const char *name_value,
                           const char *name_array, WORD_LIST *extra);

int ate_action_index_rows (const char *name_handle, const char *name_value,
                           const char *name_array, WORD_LIST *extra);

int ate_action_get_field_sizes(const char *name_handle, const char *name_value,
                               const char *name_array, WORD_LIST *extra);

int ate_action_sort(const char *name_handle, const char *name_value,
                         const char *name_array, WORD_LIST *extra);

int ate_action_walk_rows(const char *name_handle, const char *name_value,
                         const char *name_array, WORD_LIST *extra);

/** @} */

/**
 * @defgroup Action_Delegation Internal Action Delegation
 * @{
 */

/**
 * @brief Used by @ref ate_action_sort to deliver context information
 *        to the `qsort_r` callback function.
 *
 * For efficiency, some resources are allocated before calling
 * `qsort_r` and passed along to the comparison function to save
 * repeated existence test and allocation of reusable shell variables.
 */
struct qsort_package {
   int row_size;              ///< number of elements to put in comparison arrays
   SHELL_VAR *callback_func;  ///< script callback function to make the comparison
   SHELL_VAR *return_var;     ///< pre-allocated return value shell variable
   const char *name_left;     ///< pre-allocated left comparison shell variable
   const char *name_right;    ///< pre-allocated right comparison shell variable
};

/**
 * @brief Used to map action strings to the action functions for
 *        @ref delegate_action and as a source of help display content
 *        for @ref delegate_list_actions and @ref delegate_show_action_usage.
 */
typedef struct action_agent {
   const char *name;          ///< string used to invoke an action
   ATE_ACTION action;         ///< pointer to function handling the action
   const char *description;   ///< describes action's objective
   const char *usage;         ///< usage string
} ATE_AGENT;

/* The following functions are found in ate_delegate.c */
void delegate_list_actions(void);
void delegate_show_action_usage(const char *action_name);

int delegate_action(const char *name_action,
                    const char *name_handle,
                    const char *name_value,
                    const char *name_array,
                    WORD_LIST *extra);
/** @} */


#endif
