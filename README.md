# Custom Bash Builtin - `ate`xs

## OVERVIEW

`ate` is Array Table Extension, which creates and uses
high-performance vector indexes to elements of Bash arrays,
permitting instant access to elements for reading and writing,
fast sorting and searching using binary-searches for sorted
tables.

## FEATURES

- **Bash Builtin**
  Bash builtins can avoid the performance penalty of starting child
  processes, and can initiate script responses through callbacks to
  script functions.

- **C-language Vector of Pointers**
  Using fixed-length memory C structs containing pointers to the Bash
  array elements permits access to the Bash array elements by index
  through simple  multiplication.

- **Establishes a Virtual Table**
  The pointers of the vector can simulate a table by pointing to
  Bash array elements at fixed increments.  For example, a table
  whose rows contain three fields will have pointers to every
  third Bash array element.

- **Use Bash Memory Management**
  This is a mixed blessing.  Using Bash variables and Bash arrays
  for the data permits Bash garbage collection to manage their
  lifetimes.  The downside is that, without destructors, it is
  difficult or impossible to guarantee an orderly release of
  no-longer needed memory.  Thus the indexes are not dynamic.
  That is, a table length is fixed until it is explicitely reindexed.

- **Library**
  I'm experimenting with an interface for including Bash script
  fragments of `ate` solutions and utilities into new Bash scripts.
  The library interface provides a list of available scriptlets,
  basic usage info about each scriptlet, and a system for managing
  includes without duplicates.

## INSTALLATION

### Prerequisites

Ensure you have the following:

- Bash version `4.0.0` or higher
- A C-compiler (gcc) for compiling the command
- _bash_builtins_ development package
- _git_ to clone, build, and statically link a
  few library dependencies

The development environment is Linux.  I should work on any Linux
distribution, and will probably work on BSD as well (I haven't tested
it there yet).

### Installation Steps

1. Clone the repository:
    ```bash
    git clone https://github.com/cjungmann/ate.git
    ```
2. Navigate to the cloned directory:
    ```bash
    cd ate
    ```
3. Run the `configure` script to resolve dependencies and prepare the Makefile.
   ```bash
   ./configure
   ```
3. Build and install the builtin:
    ```bash
    make
    sudo make install
    ```
4. Verify the installation by running:
    ```bash
    enable ate
    ate list_actions
    ```

## USAGE

### Syntax
```bash
ate [action_name] [handle_name] [options]
```

### Example

The following example will make and display an `ate` two-column
table of file names and sizes:

~~~bash
enable ate
source <( ate_sources ate_exit_on_error ate_print_formatted_table )

# Make a two-field table
declare AHANDLE
ate declare AHANDLE 2
ate_exit_on_error

declare name
while IFS='|' read -r -a row; do
   ate append_data AHANDLE "${row[@]}"
done < <( stat -c"%n|%s" * )

ate index_rows AHANDLE
ate_exit_on_error

ate_print_formatted_table AHANDLE
~~~





