/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

#include <hardware.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS    25

Byte x, y=19;
Byte current_color = 0x02; /* default: green on black (fg=2,bg=0) */

void printc(char c)
{
  bochs_out(c);
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | ((Word)current_color << 8);
    Word *screen = (Word *)0xb8000;
    screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
  }
}

void printc_color(char c, Byte color)
{
  bochs_out(c);
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    /* color here is treated as combined attribute: (bg<<4)|fg or just fg in low nibble */
    Byte attr = ((color & 0xFF) & 0xFF);
    Word ch = (Word) (c & 0x00FF) | ((Word)attr << 8);
    Word *screen = (Word *)0xb8000;
    screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
  }
}

void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx=x;
  cy=y;
  x=mx;
  y=my;
  printc(c);
  x=cx;
  y=cy;
}

void screen_set_pos(Byte mx, Byte my)
{
  if (mx < NUM_COLUMNS && my < NUM_ROWS) {
    x = mx;
    y = my;
  }
}

void screen_set_color(Byte fg, Byte bg)
{
  Byte attr = ((bg & 0x0F) << 4) | (fg & 0x0F);
  current_color = attr;
}

Byte screen_get_color(void)
{
  return current_color;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}
