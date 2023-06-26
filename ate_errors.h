#ifndef ATE_ERRORS_H
#define ATE_ERRORS_H

int ate_error_missing_action(void);
int ate_error_missing_arguments(const char *action_name);
int ate_error_arg_not_number(const char *arg_value);
int ate_error_not_found(const char *what);
int ate_error_action_not_found(const char *action_name);
int ate_error_function_not_found(const char *func_name);
int ate_error_var_not_found(const char *var_name);
int ate_error_wrong_type_var(SHELL_VAR *var, const char *type);
int ate_error_missing_usage(const char *what);
int ate_error_failed_to_create(const char *what);
int ate_error_corrupt_table(void);
int ate_error_invalid_row_size(int requested);
int ate_error_record_out_of_range(int requested, int limit);

#endif
