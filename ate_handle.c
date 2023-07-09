/**
 * @file ate_handle.c
 * @brief Code here manages tasks around allocating memory, identifying
 *        by type, creating virtual row indicies.
 */

#include "ate_handle.h"
#include "ate_utilities.h"

static const char AHEAD_ID[] = "ATE_HANDLE";

/**
 * @brief Identify AHEAD SHELL_VAR
 * @param "var"   SHELL_VAR to be identified
 * @return True if `var` is a AHEAD, False if not
 *
 * Since attribute `att_special` is not specific, identifying an AHEAD
 * is a two-step process, confirm `att_special` attribute, then cast
 * and confirm the identifying char*.
 */
bool ahead_p(const SHELL_VAR *var)
{
   if (specialvar_p(var))
      return (ahead_cell(var))->typeid == AHEAD_ID;

   return False;
}


/**
 * @brief Using function to name the calculation.
 * @param "row_count"  number of index entries for which to reserve memory
 * @return Number of bytes needed to a AHEAD struct with sufficient space
 *         to accommodate the expected rows.
 */
size_t ate_calculate_head_size(int row_count)
{
   if (row_count > 0)
      return sizeof(AHEAD) + (size_t)row_count * sizeof(ARRAY_ELEMENT*);
   else
      return sizeof(AHEAD);
}

/**
 * @brief Convenient dereferencing to access element count;
 * @param "head"   Handle to initialized AHEAD
 * @return -1 for unavailable, otherwise returns number of elements
 */
int ate_get_element_count(const AHEAD *head)
{
   if (head->array && array_p(head->array))
   {
      ARRAY *arr = array_cell(head->array);
      return array_num_elements(arr);
   }

   return -1;
}

/**
 * @brief Initialize only the AHEAD part of the memory block
 * @param "head"      pointer to memory block that should be initialized as AHEAD
 * @param "array"     SHELL_VAR hosting an ARRAY.  The scope of this variable
 *                    must be managed outside of the `ate` object because we can't
 *                    override the destructor of SHELL_VAR.
 * @param "row_size"  number of indexes to be created.
 * @return True if succeeded, False if failed.
 */
bool ate_initialize_head(AHEAD *head, SHELL_VAR *array, int row_size)
{
   if (array && array_p(array))
   {
      ARRAY *parray = array_cell(array);
      memset(head, 0, sizeof(struct ate_head));
      head->typeid = AHEAD_ID;
      head->array = array;
      if (row_size > 0)
      {
         int elcount = parray->num_elements;
         if ((elcount % row_size) == 0)
            head->row_size = row_size;
      }
      head->row_count = 0;
      return True;
   }

   return False;
}

/**
 * @brief Populates the array of ARRAY_ELEMENT pointers with pointers to
 *        the first element of a virtual row, as indicated by the number
 *        of elements in a row as indicated by the @p row_size value.
 *
 * @param "target"   the memory block containing the uninitialized pointers
 * @param "source"   the ARRAY variable whose elements are to be indexed
 * @param "row_size" number of elements in a virtual row
 * @param "row_count" the number of ARRAY pointer elements included in
 *                    the @p target memory block
 * @return True if succeeded, False if failed.
 *
 * It assumed that the @p row_count value accurately reports the number
 * of ARRAY_ELEMENT pointers allocated at the end of the AHEAD struct.
 * The @p row_count value should have been calculated earlier as the
 * number of ARRAY elements divided by the number of elements in a row,
 * without a remainder.
 */
bool ate_initialize_row_pointers(AHEAD *target, SHELL_VAR *source, int row_size, int row_count)
{
   // Get array pointers, including head to detect completion of walk
   ARRAY_ELEMENT *head = (array_cell(source))->head;
   ARRAY_ELEMENT *optr = head->next;

   ARRAY_ELEMENT **nptr = (ARRAY_ELEMENT**)target->rows;

   int cur_el_index = 0;
   int cur_row_number = 0;

   while (optr != head)
   {
      if ((cur_el_index % row_size)==0)
      {
         *nptr++ = optr;
         ++cur_row_number;
      }

      optr = optr->next;
      ++cur_el_index;
   }

   return cur_row_number == row_count;
}

/**
 * @brief Allocate and initialize a AHEAD struct in a contiguous memory block,
 *        suitable for saving to a SHELL_VAR::value member and will require no
 *        deallocation processes beyond simply freeing the memory.
 *
 * @param "head"     [out] pointer-to-pointer through which the new AHEAD memory
 *                         block will be returned.
 * @param "array"    [in]  variable declared as an array with `-a`
 * @param "row_size" [in]  number of elements per row to use when calculating
 *                         and assigning ARRAY_ELEMENT pointers
 *
 * @return True if succeeded, False if failed.
 */
bool ate_create_indexed_head(AHEAD **head, SHELL_VAR *array, int row_size)
{
   if (array && array_p(array))
   {
      int el_count = (array_cell(array))->num_elements;
      if ((el_count % row_size)==0)
      {
         // Get block memory to hold AHEAD and calculated number
         // of pointers to ARRAY_ELEMENT for indexed access
         int row_count = el_count / row_size;
         size_t mem_required = ate_calculate_head_size(row_count);
         AHEAD *temp_head = (AHEAD*)xmalloc(mem_required);

         if (temp_head)
         {
            if (ate_initialize_head(temp_head, array, row_size))
            {
               if (ate_initialize_row_pointers(temp_head, array, row_size, row_count))
               {
                  temp_head->row_count = row_count;
                  *head = temp_head;
                  return True;
               }
            }

            // Discard memory if still running this function
            xfree(temp_head);
         }
      }
   }
   return False;
}

/**
 * @brief Manage reuse or creation of new SHELL_VAR for a handle
 *
 * Create or reuse a SHELL_VAR as a handle containing a AHEAD
 * instance.  For actions that create a handle without using it,
 * passing NULL to the @p handle argument will allow the calling
 * function to omit allocating an unnecessary pointer variable.
 *
 * @param "handle"  [out]  where new handle is returned.  Pass a NULL
 *                         value if the handle is not needed for use.
 * @param "name"    [in]   name for the new SHELL_VAR
 * @param "head"    [in]   an initialized AHEAD memory block
 * @return False if failed to secure an appropriate SHELL_VAR
 */
bool ate_install_head_in_handle(SHELL_VAR **handle,
                               const char *name,
                               AHEAD *head)
{
   SHELL_VAR *newvar = NULL;
   SHELL_VAR *svar = find_variable(name);
   if (svar == NULL)
      svar = newvar = bind_variable(name, "", att_special);

   if (svar)
   {
      ate_dispose_variable_value(svar);
      svar->value = (char*)head;
      svar->attributes |= att_special;

      // only return new svar if we have somewhere to save it:
      if (handle)
         *handle = svar;

      return True;
   }

   if (newvar)
      dispose_variable(newvar);

   return False;
}

/**
 * @brief Create and return a new SHELL_VAR handle for an array
 *
 * @param "retval"   [out] address for access to the new handle SHELL_VAR
 * @param "name"     [in]  name to use for new SHELL_VAR
 * @param "array"    [in]  array to use as table's source
 * @param "row_size" [in]  number of elements for each table row
 */
bool ate_create_handle(SHELL_VAR **retval,
                       const char *name,
                       SHELL_VAR *array,
                       int row_size)
{
   AHEAD *head = NULL;
   if (ate_create_indexed_head(&head, array, row_size))
   {
      if (ate_install_head_in_handle(retval, name, head))
         return True;

      xfree(head);
   }

   return False;
}

/**
 * @brief Returns an ARRAY_ELEMENT indicated by index
 * @param "handle"  an initialized AHEAD handle pointer
 * @param "index"  row number requested
 * @return The indicated ARRAY_ELEMENT*, null if out-of-range
 *
 * Assumed by be called by code that has previously acquired
 * the @ref AHEAD pointer from a SHELL_VAR.
 */
ARRAY_ELEMENT* ate_get_indexed_row(AHEAD *handle, int index)
{
   if (index < handle->row_count)
      return handle->rows[index];
   else
      return (ARRAY_ELEMENT*)NULL;
}

/**
 * @brief Returns the ARRAY's head ARRAY_ELEMENT
 * @param "handle" an initialized AHEAD handle pointer
 * @return the indicated ARRAY_ELEMENT*, null @p handle is NULL
 *
 * Assumed by be called by code that has previously acquired
 * the @ref AHEAD pointer from a SHELL_VAR.
 */
ARRAY_ELEMENT* ate_get_array_head(AHEAD *handle)
{
   if (handle->array)
      return (array_cell(handle->array))->head;
   else
      return (ARRAY_ELEMENT*)NULL;
}

/**
 * @brief Test several AHEAD characteristics to see if it's consistent
 * @param "head"   Supposedly initialzed AHEAD struct
 * @return EXECUTION_SUCCESS if everything is good,
 *         non-zero if anything fails to check out
 *
 * This test is primarily for @ref reindex_array_elements.
 * @ref reindex_array_elements does a very invasive reordering of
 * elements, potentially causing ARRAY_ELEMENT orphans which
 * would then be memory leaks.
 *
 * @ref reindex_array_elements will refuse to run if this test
 * reveals any discrepencies.
 */
int ate_check_head_integrity(AHEAD *head)
{
   int retval = EXECUTION_SUCCESS;

   // Ensure self-identifies as a AHEAD
   if (head->typeid != AHEAD_ID)
   {
      retval = EX_NOTFOUND;
      goto early_exit;
   }

   // Perhaps unnecessary test that an array is attached:
   ARRAY *array = NULL;
   if (head->array == NULL
       || (array=array_cell(head->array)) == NULL)
   {
      retval = EX_NOTFOUND;
      goto early_exit;
   }

   int element_count = array->num_elements;

   // Two orphans tests:
   // Test incomplete row orphans:
   if (element_count % head->row_size)
   {
      retval = EX_BADASSIGN;
      goto early_exit;
   }
   // Test that all elements are in a row
   if (head->row_size * head->row_count != element_count)
   {
      retval = EX_NOINPUT;
      goto early_exit;
   }

     early_exit:
   return retval;
}
