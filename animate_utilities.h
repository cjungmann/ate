#ifndef ANIMATE_UTILITIES
#define ANIMATE_UTILITIES

#include "ate_handle.h"
#include <stdbool.h>
#include <termios.h>    // struct termios

typedef struct animate_parameters {
   AHEAD *table;
   AHEAD *index;

   int top_row;
   int selected_row;

   int reserve_top;
   int reserve_bottom;
   int reserve_left;
   int reserve_right;

   // These valkes are calculated from screen size and
   // the reserve_ members:
   int top_line;     ///< screen line to begin plotting data
   int left_cell;    ///< screen cell to begin plotting a line
   int line_count;   ///< number of screen lines to show
   int line_width;   ///< number of chars between left- and right-margins
} APARAMS;


void cf_make_raw(struct termios* tos);

void reset_screen(void);
bool get_echo_status(void);
bool set_echo(bool on);

bool get_cursor_position(int *row, int *col);
bool set_cursor_position(int row, int col);
void get_screen_size(int *wide, int *tall);

void animate(APARAMS *params);

void update_data_boundaries(APARAMS *params);
void erase_data_window(APARAMS *params);
void replot_data(APARAMS *params);



#endif
