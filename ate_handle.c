#include "ate_handle.h"

static const char AHEAD_ID[] = "ATE_HANDLE";


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
 * @param "handle"   [out] pointer-to-pointer through which the new AHEAD memory
 *                         block will be returned.
 * @param "array"    [in]  variable declared as an array with `-a`
 * @param "row_size" [in]  number of elements per row to use when calculating
 *                         and assigning ARRAY_ELEMENT pointers
 *
 * @return True if succeeded, False if failed.
 */
bool ate_create_indexed_handle(AHEAD **handle, SHELL_VAR *array, int row_size)
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
         AHEAD *head = (AHEAD*)xmalloc(mem_required);

         if (head)
         {
            if (ate_initialize_head(head, array, row_size))
            {
               if (ate_initialize_row_pointers(head, array, row_size, row_count))
               {
                  head->row_count = row_count;
                  *handle = head;
                  return True;
               }
            }

            xfree(head);
         }
      }
   }
   return False;
}
