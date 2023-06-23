# PROJECT ARRAY TABLE EXTENSION

Turn a Bash array into a quick-access virtual table.

## INTRODUCTION

This builtin module enhances Bash arrays in two major ways:
It allows for direct random access to elements, and it provides an
interface through which the array will present as a table.

## USAGE

This a preliminary explanation as the interface is still being
developed.

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

Download or clone the code, then run `make`.  A successful build will
generate `ate.so`.


## ALTERNATE NAMES

I'm not really happy with the name *ate*, but I'm moving on for now.
I've also considered `bat` for **Bash Array Table**, but decided
against competing with another possily popular app, [bat][bat].

Using `ate` for now, but alternative names (in case `ate` seems
inappropriate) might be `bia` for **Bash Indexed Array** or `iat`
for **Indexed Array Table**.






[bat]:  "https://github.com/sharkdp/bat"