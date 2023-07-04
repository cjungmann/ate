#ifndef ATE_HANDLE_H
#define ATE_HANDLE_H

#include <builtins.h>

// Prevent multiple inclusion of shell.h:
#ifndef EXECUTION_FAILURE
#include <shell.h>
#endif

#include <builtins/bashgetopt.h>  // for internal_getopt(), etc
#include <builtins/common.h>      // for no_options()

typedef unsigned int bool;
#define True (1)
#define False (0)

typedef struct ate_head {
   const char *typeid;     ///< pointer to string array for confirming att_special type
   SHELL_VAR *array;       ///< array to which @p row elements will point
   int row_size;           ///< number of elements in a row
   int row_count;          ///< number of @p rows elements in structure
   ARRAY_ELEMENT *rows[];  ///< beginning of array of pointers
} AHEAD;

/**
 * @brief Echo SHELL_VAR identification and cast conventions for AHEAD
 * @{
 */
bool ahead_p(const SHELL_VAR *var);
#define ahead_cell(var) (AHEAD*)((var)->value)
/** @} */

/**
 * @brief Informational AHEAD methods
 * @{
 */
size_t ate_calculate_head_size(int row_count);
int ate_get_element_count(const AHEAD *head);
/** @} */

/**
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
