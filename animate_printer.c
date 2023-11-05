#include "pwla.h"

#include <stdio.h>
#include <termios.h>    // tcgetattr()
#include <unistd.h>
#include <assert.h>

#include "ate_handle.h"
#include "ate_utilities.h"
#include "ate_errors.h"

#include "pwla_walk_rows.h"

#include "word_list_stack.h"

#include "animate_utilities.h"

void update_data_boundaries(APARAMS *params)
{
   int wide, tall;
   get_screen_size(&wide, &tall);
   params->top_line = params->reserve_top + 1;
   params->left_cell = params->reserve_left + 1;
   params->line_count = tall - (params->reserve_top + params->reserve_bottom);
   params->line_width = wide - ( params->reserve_left + params->reserve_right);
}

void erase_data_window(APARAMS *params)
{
   // Catch uninitialized params
   assert(params->line_width);

   char buff[10];
   int cmdlen = snprintf(buff, sizeof(buff), "\x1b[%dX", params->line_width);
   int fh = STDIN_FILENO;

   struct termios original, raw;
   tcgetattr(fh, &original);
   raw = original;
   cf_make_raw(&raw);
   tcsetattr(fh, TCSANOW, &raw);

   char cmd[]="\x1b#8";
   write(fh, cmd, strlen(cmd));
   printf("erare chars string is '^[%s'.\n", &buff[1]);

   int line_limit = params->top_line + params->line_count;
   for (int i=params->top_line; i<line_limit; ++i)
   {
      set_cursor_position(i, params->left_cell);
      write(fh, buff, cmdlen);
   }

   tcsetattr(fh, TCSANOW, &original);
}



void replot_data(APARAMS *aparams)
{
   int indexed = aparams->index ? 1 : 0;

   AHEAD *table, *walker;

   ARRAY_ELEMENT **table_origin, **table_limit;
   ARRAY_ELEMENT **walker_origin, **walker_limit;

   table = aparams->table;
   table_origin = &table->rows[0];
   table_limit = table_origin + table->row_count;

   if (indexed)
   {
      walker = aparams->table;
      walker_origin = &walker->rows[0];
      walker_limit = walker_origin + walker->row_count;
   }
   else
   {
      walker = table;
      walker_origin = table_origin;
      walker_limit = table_limit;
   }

   // Setup parameters for printing the screen:
   ARRAY_ELEMENT **ptr = NULL, **limit = NULL;
   ptr = walker_origin + aparams->top_line;
   if (ptr < walker_limit)
   {
      limit = ptr + aparams->line_count;
      if (limit >= walker_limit)
         limit = walker_limit;
   }

   printf("About to print %ld out of %ld lines\n",
          limit - ptr,
          table_limit - table_origin);

   ARRAY_ELEMENT *row_ptr;
   if (ptr && limit)
   {
      while (ptr < limit)
      {
         row_ptr = NULL;
         if (indexed)
         {
            int ndx;
            get_int_from_string(&ndx, (*ptr)->next->value);
            if (ndx>=0 && ndx<table->row_count)
               row_ptr = table_origin[ndx];
         }
         else
            row_ptr = *ptr;

         printf("%s\n", row_ptr->value);

         ++ptr;
      }
   }
}
