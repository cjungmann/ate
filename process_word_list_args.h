#ifndef PROCESS_WORD_LIST_ARGS_H
#define PROCESS_WORD_LIST_ARGS_H

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#define DEFAULT_VALUE_NAME "ATE_VALUE"
#define DEFAULT_ARRAY_NAME "ATE_ARRAY"

typedef enum {
   AL_ARG = 1,
   AL_OPT
} AL_TYPE;

typedef struct arg_target ARG_TARGET;
typedef struct arg_values ARG_LIST;

struct arg_target {
   const char *name;
   AL_TYPE    type;
   const char **value;
   SHELL_VAR **shellvar;
   const char *default_value;
};

struct arg_values {
   const char *value;
   ARG_LIST *next;
};

#define args_from_word_list(AFWL_ARGS, AFWL_WL)    \
   ARG_LIST *AFWL_ARG_NEW;                                              \
   ARG_LIST *AFWL_ARG_TAIL = (AFWL_ARGS) = (ARG_LIST*)alloca(sizeof(ARG_LIST)); \
   memset(AFWL_ARG_TAIL,0,sizeof(ARG_LIST));                                \
   WORD_LIST *AFWL_PTR=(AFWL_WL);                        \
   while(AFWL_PTR)                                       \
   {                                                     \
      AFWL_ARG_NEW = (ARG_LIST*)alloca(sizeof(ARG_LIST));   \
      AFWL_ARG_NEW->value = AFWL_PTR->word->word;           \
      AFWL_ARG_NEW->next = NULL;                            \
      AFWL_ARG_TAIL->next = AFWL_ARG_NEW;                   \
      AFWL_ARG_TAIL = AFWL_ARG_NEW;                         \
      AFWL_PTR = AFWL_PTR->next;                            \
   }

typedef enum {
   AL_NONE = 0,         ///< no flags
   AL_NOTIFY_MISSING,   ///< notify for any missing arguments
   AL_NOFITY_UNKNOWN,   ///< notify for unknown options
   AL_END               ///< bounds-confirming value
} AL_FLAGS;

ARG_TARGET *pwla_find_option_target(ARG_TARGET *targets, char option);
ARG_TARGET *pwla_next_arg_target(ARG_TARGET *targets);
int process_word_list_args(ARG_TARGET *targets, ARG_LIST *args_handle, AL_FLAGS flags);

void dump_targets(ARG_TARGET *targets, const char *action);

int pwla_declare(ARG_LIST *alist);
int pwla_append_data(ARG_LIST *alist);
int pwla_index_rows(ARG_LIST *alist);
int pwla_get_row_count(ARG_LIST *alist);
int pwla_get_row_size(ARG_LIST *alist);
int pwla_get_array_name(ARG_LIST *alist);
int pwla_get_field_sizes(ARG_LIST *alist);

int pwla_get_row(ARG_LIST *alist);
int pwla_put_row(ARG_LIST *alist);


// in pwla_walk_rows.c:
int pwla_walk_rows(ARG_LIST *alist);

int pwla_sort(ARG_LIST *alist);


int test_pwla(WORD_LIST *wl);



#endif
