#include "animate_utilities.h"

#include <stdio.h>
#include <stdlib.h>

// #include <sys/ioctl.h>  // for ioctl()
//                         // SEE ioctl(2),
//                         //    ioctl_console(2), and
//                         //    ioctl_tty(2)
// #include <errno.h>
#include <termios.h>    // tcgetattr()
#include <unistd.h>

/**
 * @brief Change settings to raw terminal io mode
 *
 * Use `tcgetattr` for current settings, the `cf_make_row` to modify,
 * then `tcsetattr` to use the raw settings.

 * Code borrowd from not-guaranteed C library function.
 * cfmakeraw().
 *
 * @param "tos"  pointer to struct with valid termios values
 *
 * @code(c)
 * struct termios original, raw;
 * tcgetattr(STDOUT_FILENO, &original);
 * // Preserve original settings with copy to modify: 
 * raw = original;
 * cf_make_raw(&raw);
 * tcsetattr(STDOUT_FILENO, &raw);
 * work_with_row();
 * tcsetattr(&original);
 * @endcode
 */
void cf_make_raw(struct termios* tos)
{
   tos->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                           |INLCR|IGNCR|ICRNL|IXON);

   tos->c_oflag &= ~OPOST;

   tos->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);

   tos->c_cflag &= ~(CSIZE|PARENB);

   tos->c_cflag |= CS8;
}

void reset_screen(void)
{
   fputs("\x1b[2J\x1b[1;1H", stdout);
}

/**
 * @brief Returns whether terminal echo is on (True) or off (False).
 */
bool get_echo_status(void)
{
   struct termios term;
   int ecode;
   ecode = tcgetattr(1, &term);
   if (!ecode)
   {
      if ( term.c_lflag & ECHO )
         return true;
   }

   return false;
}

/**
 * @brief Sets the terminal echo state, returning previous value.
 *
 * @param "on"   is True to turn echo on, False to turn echo off.
 *
 * @return State of echo before the function was called
 */
bool set_echo(bool on)
{
   bool previous_status = false;
   int fd = 1;

   struct termios term;
   int ecode;
   ecode = tcgetattr(fd, &term);
   if (!ecode)
   {
      if ( term.c_lflag & ECHO )
         previous_status = true;

      if ( on )
         term.c_lflag |= ECHO;
      else
         term.c_lflag &= ~ECHO;

      ecode = tcsetattr(fd, 0, &term);
   }

   return previous_status;
}

#define CMD_CPR "\x1b[6n"   // Cursor Position Report

bool get_cursor_position(int *row, int *col)
{
   if (row==NULL || col==NULL)
      goto exit_on_error;

   int fhandle = STDIN_FILENO;

   struct termios saved, raw;
   tcgetattr(fhandle, &saved);
   raw = saved;
   cf_make_raw(&raw);
   tcsetattr(fhandle, TCSANOW, &raw);

   write(fhandle, CMD_CPR, sizeof(CMD_CPR)-1);

   char buff[28];
   int bytes_read = read(fhandle, buff, sizeof(buff));

   tcsetattr(fhandle, TCSANOW, &saved);

   if (bytes_read && buff[0] == '\x1b' && buff[1] == '[')
   {
      buff[bytes_read] = '\0';
      char *ptr = &buff[2];
      sscanf(ptr, "%d;%dR", row, col);
      return 1;
   }

  exit_on_error:
   return 0;
}

bool set_cursor_position(int row, int col)
{
   char reqbuff[15];
   int bytes_copied = snprintf(reqbuff, 15, "\x1b[%d;%dH", row, col);
   write(STDIN_FILENO, reqbuff, bytes_copied);

   return 1;
}

void get_screen_size(int *wide, int *tall)
{
   int o_row, o_col;
   get_cursor_position(&o_row, &o_col);
   set_cursor_position(999, 999);
   get_cursor_position(tall, wide);
   set_cursor_position(o_row, o_col);
}

