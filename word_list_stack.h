/**
 * @file word_list_stack.h
 * @brief Macros to create or augment WORD_LISTs in stack memory
 *
 * This set of macros allows the developer to quickly create an
 * arbitrary WORD_LIST in stack memory.
 *
 * @ref WORD_LIST_Macros
 */

#ifndef WORD_LIST_STACK_H
#define WORD_LIST_STACK_H

#include <command.h>              // for COMMAND.flags, among other things
#include <alloca.h>


/**
 * @defgroup WORD_LIST_Macros Macros for Stack-based WORD_LIST
 *
 * @brief Set of macros for creating or extending a WORD_LIST in
 *        stack memory.
 *
 * The primary macros from this group are @ref WL_WALK and
 * @ref WL_APPEND.
 *
 * @anchor WLE_Def
 * The macros create "WORD_LIST entries," otherwise known as
 * **WLE** blocks.  A WLE is a single block of memory containing a
 * WORD_LIST struct, a WORD_DESC struct, and bytes to contain a
 * NULL-terminated string.  After initialization, the `word` member
 * of the WORD_LIST struct points to the WORD_DESC struct, whose
 * `word` member points to the string at the end of the WLE.
 *
 * @anchor WLE_Example
~~~c
void append(WORD_LIST *list)
{
   // find the tail
   WORD_LIST *tail = list;
   while (tail->next)
      tail = tail->next;

   const char* newels[] = {
      "word_one",
      "word_two",
      NULL
   };

   constchar **strarray = newels;
   WL_WALK(list, tail, strarray);

   use_the_word_list(list);

   // restore existing WORD_LIST:
   tail->next = NULL;
}
~~~
 * @{
 */

/**
 * @brief Returns size required for a @ref WLE_def.
 * Calculates the size needed for a single block of memory that will
 * have pointer members initialized to addresses within the block,
 * leaving enough room at the end to contain the string.
 * @param "STR"   char* to zero-terminated string to be accommodated
 * @return number of bytes required for WORD_LIST, WORD_DESC and string data
 */
#define WL_SIZE(STR) sizeof(WORD_LIST) + sizeof(WORD_DESC) + strlen((STR)) + 1

/**
 * @brief Allocates from the stack a zero-filled memory block
 * @param "STR"   String value, the length of which informs the
 *                required memory block size.
 * @return Zero-filled memory block
 */
#define WL_ALLOC(STR) (WORD_LIST*)memset(alloca(WL_SIZE((STR))),0,WL_SIZE((STR)))

// Macros returning pointers to WORD_LIST, WORD_DESC, and char[]
/**
 * @brief Return pointer to WORD_LIST in given WLE memory block
 * @param "BLOCK"   Address of WLE memory block
 * @returns pointer to WORD_LIST section of WLE block
 */
#define WL_P_LIST(BLOCK) (WORD_LIST*)(BLOCK)

/**
 * @brief Return pointer to WORD_DESC in given WLE memory block
 * @param "BLOCK"   Address of WLE memory block
 * @returns pointer to WORD_DESC section of WLE block
 */
#define WL_P_DESC(BLOCK) (WORD_DESC*)(((char*)(BLOCK))+sizeof(WORD_LIST))

/**
 * @brief Return pointer to the char string in a given @ref WLE_def memory block
 * @param "BLOCK"   Address of WLE memory block
 * @returns pointer to string
 */
#define WL_A_STR(BLOCK) (char*)((char*)(BLOCK)+sizeof(WORD_LIST)+sizeof(WORD_DESC))

// Macros to initialize structs with appropriate pointers
/**
 * @brief Sets `word` member of WORD_LIST to address of WORD_DESC in WLE block
 * @param "BLOCK"   Address of WLE memory block
 */
#define WL_INITLIST(BLOCK) (WL_P_LIST((BLOCK)))->word = WL_P_DESC((BLOCK))

/**
 * @brief Sets `word` member of WORD_DESC to address of string in WLE block.
 * @param "BLOCK"   Address of WLE memory block
 */
#define WL_INITDESC(BLOCK) (WL_P_DESC((BLOCK)))->word = WL_A_STR((BLOCK))

/**
 * @brief Copies string contents to string buffer in WLE block
 * @param "BLOCK"   Address of WLE memory block
 * @param "STR"     String value to copy
 *
 * @note This macro should only be used on a block that was previously
 *       allocated using @ref WL_ALLOC to ensure adequate buffer space
 *       to safely copy the string.
 */
#define WL_COPYWORD(BLOCK, STR) strcpy(WL_A_STR((BLOCK)),(char*)(STR))

/**
 * @brief Copies string and sets pointers for structs in WLE block.
 * @param "BLOCK"   Address of WLE memory block
 * @param "STR"     String value to copy
 *
 * Sets pointers for WORD_LIST and WORD_DESC structs while copying the
 * string.
 */
#define WL_INITBLOCK(BLOCK,STR) WL_INITLIST((BLOCK)); WL_INITDESC((BLOCK)); WL_COPYWORD((BLOCK),(STR))

/**
 * @brief Add a link to the end of a WORD_LIST chain
 * @param "TAIL"   pointer to end of WORD_LIST chain
 * @param "WORD"   char pointer to word value to store
 */
#define WL_APPEND(TAIL, WORD) {           \
   WORD_LIST *NEWTAIL = WL_ALLOC((WORD)); \
   if (NEWTAIL)                           \
   {                                      \
      WL_INITBLOCK(NEWTAIL, (WORD));      \
      if ((TAIL) != NULL)                 \
         (TAIL)->next = NEWTAIL;          \
      (TAIL) = NEWTAIL;                   \
   }                                      \
}

/**
 * @brief Macro to walk through the array of strings to build the WORD_LIST.
 * @param "HEAD"  name of WORD_LIST pointer, start at NULL for new list,
 *                or has the head of an in-progress WORD_LIST.
 * @param "TAIL"  name of WORD_LIST pointer to the end of the list.  May
 *                be NULL for new list, otherwise points to the last
 *                element of an in-progress WORD_LIST.
 * @param "ARREL" pointer to a NULL-terminated array of strings from which
 *                the WORD_LIST will be augmented.  This "argument" should
 *                be a unique pointer to an array of strings, rather passing
 *                the array of strings directly, otherwise the array of
 *                of strings will be lost.
 */
#define WL_WALK(HEAD, TAIL, ARREL) \
while(*(ARREL)) {                  \
   WL_APPEND((TAIL), *(ARREL));    \
   if ((HEAD) == NULL)             \
      (HEAD) = (TAIL);             \
   ++(ARREL);                      \
}

/** @} */






/**
 * @page WLE_def WORD_LIST Entry
 *
 * A WORD_LIST Entry (WLE) is a single block of memory comprised of
 * consecutive WORD_LIST, WORD_DESC structs followed by a
 * NULL-terminated string whose address is indicated by the `word`
 * member of the contained WORD_DESC::word member.
 */

/**
 * @page WLE_Intro WORD_LIST Entry (WLE) Intro
 * The Bash builtin library seems not to have a function for adding
 * a new link to a WORD_LIST, so this is my solution for that need.
 *
 * Further, since my usage of WORD_LISTs is ad hoc and temporary,
 * this solution creates the new or augmented WORD_LIST in stack
 * memory for ease of cleanup.
 *
 * Conceptually, this could be done with *inline* functions, but
 * since the allocations are from the stack, and I'm not sure how
 * or even if it's possible to *guarantee* a function will be inline,
 * I resorted to macros.
 *
 * @warning Using stack memory for WORD_LISTs can have unexpected
 * problems.  Look at this @ref WLE_Warning
 *
 * The best practice for appending to an existing WORD_LIST chain is
 * to save a pointer to the pre-existing tail so its `next` pointer
 * can be reset to NULL when the function returns.
 */

/**
 * @page WLE_Warning Warning About Stack-based Entries
 *
 * Be careful when using @ref WL_APPEND and @ref WL_WALK to extend
 * an existing WORD_LIST, especially if the existing WORD_LIST will
 * live on after the function returns.
 *
 * @ref WL_APPEND and @ref WL_WALK create WORD_LIST entries
 * (@ref WLE_def) in stack memory using *alloca*.  If a pre-existing
 * WORD_LIST from another stack frame or in heap memory is extended
 * with entries from one of these macros, the `next` member of the
 * last WORD_LIST element will point to an invalid memory location
 * when the function of the stack-based WORD_LIST entries returns.
 *
 * If it is only a temporary extension, save a pointer to the `tail`
 * of the pre-existing WORD_LIST so the `next` member can be
 * reset to NULL before the function terminates.
 *
 * ~~~c
void append(WORD_LIST *list)
{
   // find the tail
   WORD_LIST *tail = list;
   while (tail->next)
      tail = tail->next;

   const char* newels[] = {
      "word_one",
      "word_two",
      NULL
   };

   constchar **strarray = newels;
   WL_WALK(list, tail, strarray);

   use_the_word_list(list);

   // restore existing WORD_LIST:
   tail->next = NULL;
}
 * ~~~
 */
#endif



