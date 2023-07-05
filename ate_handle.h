#ifndef ATE_HANDLE_H
#define ATE_HANDLE_H

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()


typedef enum {
   False = 0,
   True
} bool;
// typedef unsigned int bool;
// #define True (1)
// #define False (0)


/**
 * @brief working details of a table extension to a Bash ARRAY
 *
 * The information in this struct is used to perform the table
 * extension actions of this builtin module.
 *
 * An **ate** handle SHELL_VAR will have a pointer to a memory
 * block using this signature.
 */
typedef struct ate_head {
   const char *typeid;     ///< pointer to string array for confirming att_special type
   SHELL_VAR *array;       ///< array to which @p row elements will point
   int row_size;           ///< number of elements in a row
   int row_count;          ///< number of @p rows elements in structure
   ARRAY_ELEMENT *rows[];  ///< beginning of array of pointers
} AHEAD;

/**
 * @defgroup Bash_Conventions Implementation of Bash Conventions
 * @brief Use Bash builtins model for type-confirming and extracting
 *        @ref AHEAD information from a SHELL_VAR
 * @{
 */
bool ahead_p(const SHELL_VAR *var);
#define ahead_cell(var) (AHEAD*)((var)->value)
/** @} */

/**
 * @defgroup AHEAD_info AHEAD Measuring
 * @brief Functions used to determine memory size requirements
 *        when preparing a new AHEAD instance.
 * @{
 */
size_t ate_calculate_head_size(int row_count);
int ate_get_element_count(const AHEAD *head);
/** @} */

/**
 * @defgroup AHEAD_management Creating and managing AHEADs
 * @brief AHEAD memory-block allocation and construction
 * @{
 */
bool ate_initialize_head(AHEAD *head, SHELL_VAR *array, int row_size);
bool ate_initialize_row_pointers(AHEAD *target,
                                 SHELL_VAR *source,
                                 int row_size,
                                 int row_count);

bool ate_create_indexed_head(AHEAD **head,
                             SHELL_VAR *array,
                             int row_size);

bool ate_install_head_in_handle(SHELL_VAR **handle,
                               const char *name,
                               AHEAD *head);

bool ate_create_handle(SHELL_VAR **retval,
                       const char *name,
                       SHELL_VAR *array,
                       int row_size);
/** @} */

ARRAY_ELEMENT* ate_get_indexed_row(AHEAD *head, int index);
ARRAY_ELEMENT* ate_get_array_head(AHEAD *head);





#endif
