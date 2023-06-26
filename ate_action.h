#ifndef ATE_ACTION_H
#define ATE_ACTION_H

#include <variables.h>
#include <array.h>

#include "ate_handle.h"

typedef int (*ATE_ACTION)(const char *name_handle,
                          const char *name_value,
                          const char *name_array,
                          WORD_LIST *extra);


int ate_action_show_actions(const char *name_handle, const char *name_value,
                            const char *name_array, WORD_LIST *extra);

int ate_action_declare(const char *name_handle, const char *name_value,
                       const char *name_array, WORD_LIST *extra);

int ate_action_get_row_count(const char *name_handle, const char *name_value,
                             const char *name_array, WORD_LIST *extra);

int ate_action_get_row(const char *name_handle, const char *name_value,
                       const char *name_array, WORD_LIST *extra);

int ate_action_put_row(const char *name_handle, const char *name_value,
                       const char *name_array, WORD_LIST *extra);

int ate_action_append_data(const char *name_handle, const char *name_value,
                           const char *name_array, WORD_LIST *extra);

int ate_action_walk_rows(const char *name_handle, const char *name_value,
                         const char *name_array, WORD_LIST *extra);






typedef struct action_agent {
   const char *name;
   ATE_ACTION action;
   const char *description;
   const char *usage;
} ATE_AGENT;

/** @brief Found in ate_delegate.c
 * @{
 */
void delegate_show_action_usage(void);

int delegate_action(const char *name_action,
                    const char *name_handle,
                    const char *name_value,
                    const char *name_array,
                    WORD_LIST *extra);
/** @} */


#endif
