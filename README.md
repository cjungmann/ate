# PROJECT ARRAY TABLE EXTENSION

**ate** is an acronym for (Bash) Array Table Extension

## INTRODUCTION

**ate** is a loadable builtin function for Bash that extends the
Bash language with an API (application programming interface) that
treats a Bash array as table with simple database-like functions.

**ate** works with a custom Bash shell variable that catalogs the
first array element of each virtual row in a C-langauge array that
enables nearly instant indexed access to the virtual rows for
reading and writing.

**ate** features are accessed through the **ate** command, followed
by an `action` string and parameters that control the operation of
the requested action.

## USAGE

~~~.sh
# display available ate actions:
ate list_actions

# display usage information about one or all ate actions:
ate show_action declare    # display usage for declare action
ate show_action            # display usage text for all actions
~~~



## SETUP

### Clone project, build and install:

~~~sh
clone https://www.github.com/cjungmann/ate.git
cd ate
make
sudo make install
~~~
### Enable ate

As a Bash builtin, the **ate** module must be enabled before it can
be used. 

**ate** is a Bash builtin, and must be enabled for it to be used.

Part of the installation is a script, `enable_ate` that makes it
easier to aid enable **ate**.  Put the following line in a script
that will use **ate**:

~~~sh
enable $( enable_ate )
~~~

This `enable` command can be used in a shell, a script, or in the
`.bashrc` file to enable **ate** globally.

### Basic Syntax

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


## ALTERNATE NAMES

I'm not really happy with the name *ate*, but I'm moving on for now.
I've also considered `bat` for **Bash Array Table**, but decided
against competing with another possily popular app, [bat][bat].

Other possibilities if I want to change the name might be `bia` for
**Bash Indexed Array** or `iat` for **Indexed Array Table**.




[bat]:  "https://github.com/sharkdp/bat"