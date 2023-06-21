#ifndef ATE_ACTION_H
#define ATE_ACTION_H

#include <variables.h>
#include <array.h>

#include "ate_handle.h"

typedef int (*ATE_ACTION)(const char *name_handle,
                          const char *name_value,
                          const char *name_array,
                          WORD_LIST *extra);

typedef struct action_agent {
   const char *name;
   ATE_ACTION action;
   const char *description;
} ATE_AGENT;

int delegate_action(const char *name_action,
                    const char *name_handle,
                    const char *name_value,
                    const char *name_array,
                    WORD_LIST *extra);



#endif
