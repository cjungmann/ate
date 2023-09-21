/**
 * @file pwla.h
 * @brief Header file for various pwla (`process word list arguments`) features
 */

#ifndef PROCESS_WORD_LIST_ARGS_H
#define PROCESS_WORD_LIST_ARGS_H

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#define DEFAULT_VALUE_NAME "ATE_VALUE"
#define DEFAULT_ARRAY_NAME "ATE_ARRAY"
#define DEFAULT_OUTCOME_NAME "ATE_SEEK_OUTCOME"

/**
 * @defgroup PWLA PWLA Data and Function Definitions
 * @brief Improved WORD_LIST Argument Processing Code
 * @{
 */


typedef enum {
   AL_CONSUMED = 0, ///< if target no longer eligible for selection
   AL_ARG,          ///< positional string argument
   AL_OPT,          ///< option that take an argument
   AL_FLAG,         ///< option that takes no argument
   AL_REUSE = 32    ///< set if the target can be matched mulitple times
} AL_TYPE;

typedef struct arg_target ARG_TARGET;
typedef struct arg_values ARG_LIST;

struct arg_target {
   const char *name;
   AL_TYPE    type;
   const char **value;
};

struct arg_values {
   const char *value;
   ARG_LIST *next;
};

/**
 * @brief Convenience macro for creating a stack-based ARG_LIST list linked list.
 *
 * This simplifies what would otherwise be a tedious process of
 * copying the contents of a WORD_LIST to a simpler ARG_LIST array
 * into a stack-based array.  Doing it as a macro is the easiest way
 * to do this, though a function-based solution could be done by
 * calculating the memory need and handing the memory to a function
 * that would prepare the memory.
 *
 * The argument-handling concept I'm exploring here discards list
 * elements as they are consumed.  Having the elements in stack memory
 * prevents memory leaks.
 *
 * One important aspect of the generated list is that the first element
 * of the ARG_LIST array is a non-data member that points to the first
 * element.  This is needed so we can discard the first element without
 * losing a handle on the memory.
 */
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
   AL_NOTIFY_UNKNOWN,   ///< notify for unknown options
   AL_END               ///< bounds-confirming value
} AL_FLAGS;

/**
 * @brief Container for action information to support selection and usage info
 */
struct pwla_action_def {
   const char *name;               ///< name for searching the vector
   const char *desc;               ///< description for usage messages
   const char *usage;              ///< usage string
   int (*func)(ARG_LIST *alist);   ///< function pointer to action
};


/**
 * @defgroup PWLA_Basics PWLA argument-processing utilities
 * @brief These functions found in pwla.c
 * @{
 */
ARG_TARGET *pwla_find_option_target(ARG_TARGET *targets, char option);
ARG_TARGET *pwla_next_arg_target(ARG_TARGET *targets);
int process_word_list_args(ARG_TARGET *targets, ARG_LIST *args_handle, AL_FLAGS flags);

void dump_targets(ARG_TARGET *targets, const char *action);
/** @} */


/**
 * @defgroup PWLA_ActImps PWLA Action Implementations
 * @{
 */
// list_actions and show_action are in pwla_agent.c for
// access to the array of action definitions:
int pwla_list_actions(ARG_LIST *alist);
int pwla_show_action(ARG_LIST *alist);

int pwla_declare(ARG_LIST *alist);
int pwla_append_data(ARG_LIST *alist);
int pwla_index_rows(ARG_LIST *alist);
int pwla_get_row_count(ARG_LIST *alist);
int pwla_get_row_size(ARG_LIST *alist);
int pwla_get_array_name(ARG_LIST *alist);
int pwla_get_field_sizes(ARG_LIST *alist);

int pwla_get_row(ARG_LIST *alist);
int pwla_put_row(ARG_LIST *alist);

int pwla_resize_rows(ARG_LIST *alist);
int pwla_reindex_elements(ARG_LIST *alist);

// The following are each in their own source file,
// the function name plus .c extension:
int pwla_walk_rows(ARG_LIST *alist);
int pwla_sort(ARG_LIST *alist);
int pwla_filter(ARG_LIST *alist);
int pwla_make_key(ARG_LIST *alist);
int pwla_seek_key(ARG_LIST *alist);

/** @} */

/** @} <!-- PWLA --> */
#endif
