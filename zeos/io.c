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
Byte current_color = 0x02;

int screen_gotoxy(int mx, int my)
{
  if (mx < 0 || mx >= NUM_COLUMNS) return -22;
  if (my < 0 || my >= NUM_ROWS) return -22;

  x = (Byte)mx;
  y = (Byte)my;
  return 0;
}

int screen_set_color(int fg, int bg)
{
  if (fg < 0 || fg > 15) return -22;
  if (bg < 0 || bg > 15) return -22;

  current_color = (Byte)((bg << 4) | (fg & 0x0F));
  return 0;
}

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
    Word ch = (Word) (c & 0x00FF) | ((color & 0x0F) << 8);
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

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}
