#ifndef ATE_ERRORS_H
#define ATE_ERRORS_H

/**
 * @defgroup Error_Functions Centralized Error Reporting
 *
 * All error reporting should go through these functions.  This
 * makes it possible for a future version to make error-reporting
 * optional.
 * @{
 */

void save_to_error_shell_var(const char *str);
void ate_register_error(const char *format, ...);

void ate_register_variable_not_found(const char *name);
void ate_register_function_not_found(const char *name);
void ate_register_variable_wrong_type(const char *name, const char *desired_type);
void ate_register_argument_wrong_type(const char *name, const char *desired_type);
void ate_register_corrupt_table(void);
void ate_register_invalid_row_index(int requested, int available);
void ate_register_invalid_row_size(int row_size, int el_count);
void ate_register_missing_argument(const char *name, const char *action);
void ate_register_failed_to_create(const char *name);
void ate_register_unexpected_error(const char *doing);



/** @} */

#endif
