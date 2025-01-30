# ARRAY TABLE EXTENSION

**ate** is an acronym for (Bash) Array Table Extension

## DESCRIPTION

Bash arrays can use some help.

Bash arrays are double-linked lists of string values.  Unintuitively,
rather than directly accessing an element by an index, a Bash array
finds an array element by stepping through the list to find the
element with a matching index.  Bash arrays are optimized for
efficient appending and sequential reads (ie **for** loops), but are
quite slow for changes, insertions, and indexed access, especially
for large arrays.

**ate** is a loadable builtin command for Bash that enhances Bash
arrays with a table view, fast indexed access to virtual rows, and
some database features through a simple API.

**ate** works with a custom Bash shell variable that catalogs the
first array element of each virtual row in a C-langauge array that
enables nearly instant indexed access to the virtual rows for
reading and writing.

## FEATURES

- **Much faster than stanard Bash array**  
  **ate** creates an index with a C-language array of pointers to the
  first array element of each virtual row.  The index enables direct
  access to a virtual row for both reading and writing.

- **The table state persists between calls to the `ate` command**  
  **ate** uses a custom Bash SHELL_VAR, accessed through the handle
  name, to keep track of the table dimensions and row indexes.  This
  permits Bash garbage collection to manage the memory.

- **Can create alternate views to a table**  
  Secondary handles can contain sorted or filtered views of a table

- **Binary search action `seek_key` for ISAM access**  
  Fast seek action enables efficient lookup tables for some
  data processing tasks

- **Can call Bash script functions to perform per-row actions**  
  The ability of **ate** to access script functions enables flexible
  programming techniques that are the reason one chooses a script
  solution.  For example, **ate** exposes C library function `qsort`
  to script-based comparison functions.

## PREREQUISITES

Building the project requires packages **build-essential** and
**bash-builtins**.  Please install them on your system if they are
not already installed.

## INSTALLATION

### Clone project, build and install:

~~~sh
git clone https://www.github.com/cjungmann/ate.git
cd ate
make
sudo make install
~~~

There is also an _uninstall_ Makefile target to remove all traces of
the installation.

### Enable ate

**ate** is a Bash builtin, and must be enabled before it can be used.

To simplify the enable step, the project includes a script, `enable_ate`
that makes it easier to aid enable **ate**.

~~~sh
enable $( enable_ate )
~~~

This `enable` command can be used in a script, on the commnd line, or
in the `.bashrc` file to enable **ate** globally.

## USAGE

**ate** features are accessed through the **ate** command, followed
by an `action` string and parameters that control the operation of
the requested action.  Please refer to the project man pages for
more extensive usage instructions.

### Basic Operation:

<pre>
# Making a 2-field table from an array with paired values:
declare -a pets=(
  dog     bark
  cat     meow
  chicken cluck
  fish    splash
)

# Initialize the table
ate <b>declare</b> pets_handle 2 pets

# Read row #1 (index 0):
ate <b>get_row</b> pets_handle 0
echo "index 0 contains ${ATE_ARRAY[*]}"

# Update pet row's sound
ATE_ARRAY[1]="woof"
ate <b>put_row</b> pets_handle 0 ATE_ARRAY
</pre>

### Getting help:

<pre>
# Bash builtin help system:
help ate

# ate Man pages:
man 1 ate          # complete usage guide
man 7 ate          # tutorials
man 7 ate-examples

# ate actions that provide help:

# display available ate actions:
ate <b>list_actions</b>

# display usage information about one or all ate actions:
ate <b>show_action</b> declare    # display usage for declare action
ate <b>show_action</b>            # display usage text for all actions
</pre>

## Basic Syntax

**ate** *action_name* [*handle_name*] [**-v** *return_value_name*] [**-a** *return_array_name*] [...]

- *action_name*
  The only required argument.  Tells **ate** command
  what to do

- *handle_name* is required for table actions, but may be omitted for
  some informational actions

- **-v** *return_value_name*
  For actions returning a single value, the value is saved to
  **ATE_VALUE** unless overridden by the **-v** option.

- **-a** *return_array_name*
  For actions returning an array value (**get_row**), the array will
  be named **ATE_VALUE** unless overridden by the **-a** option.

- Extra arguments as needed by specific actions.


~~~.sh
enable -f ./ate.so ate

declare -a travel_list=(
0 "shirts"
0 "pants"
0 "shoes"
0 "toiletries"
0 "rain gear"
0 "book"
)

if ! ate declare tl_handle travel_list 2; then
   exit "$?"
fi

ate get_row_count tl_handle
echo "$ATE_VALUE"

~~~

## DOWNLOAD AND BUILD

Download or clone the code, then run `make` and `make install`:

~~~sh
make
sudo make install
~~~

Use the builtin in a script by including the following line:

~~~sh
#!/usr/bin/env bash

enable $( enable_ate )
~~~

## DEBUGGING

Besides having the Bash headers available, I have found it very
useful to also have a debugger-enabled version of Bash handy for
stepping through the code.

The following instructions should help you to download, build, and
link a debugger version of Bash into the **ate** development
directory.

The instruction assume that you have a `~/gits` directory for
cloning or downloading then building source code, and a
`~/work` directory for your projects.  Set those variables
separately, copy the following lines to your copy buffer (omitting
the directory declares), and paste the copy buffer to a terminal
command line.

~~~.sh
declare workdir=~/work
declare clonedir=~/gits

# Download and build your version of Bash:
cd "$clonedir"
[[ $( bash --version ) =~ version\ ([0-9.]+) ]]
declare BVER="bash-${BASH_REMATCH[1]}"
# wget https://ftp.gnu.org/gnu/bash/${BVER}.tar.gz
tar -xzf ${BVER}.tar.gz
cd "${BVER}"
./configure --enable-debugger
make

# Make link to the debugger-enabled Bash executable:
cd "${workdir}/ate"
cp -s "${clonedir}/${BVER}/bash" .
~~~

## ALTERNATE NAMES

I'm not really happy with the name *ate*, but I'm moving on for now.
I've also considered `bat` for **Bash Array Table**, but decided
against competing with another possily popular app, [bat][bat].

Other possibilities if I want to change the name might be `bia` for
**Bash Indexed Array** or `iat` for **Indexed Array Table**.




[bat]:  "https://github.com/sharkdp/bat"