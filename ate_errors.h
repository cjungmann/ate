#ifndef ATE_ERRORS_H
#define ATE_ERRORS_H

int ate_error_not_found(const char *what);
int ate_error_var_not_found(const char *name);
int ate_error_wrong_type_var(SHELL_VAR *var, const char *type);
int ate_error_missing_usage(const char *what);
int ate_error_failed_to_create(const char *what);
int ate_error_corrupt_table(void);
int ate_error_invalid_row_size(int requested);
int ate_error_record_out_of_range(int requested, int limit);

#endif
