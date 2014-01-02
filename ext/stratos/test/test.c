
#include <stdio.h>
#include <stdint.h>
#include "stratos/rect.h"

int main()
{
  sras_init(); // Initialize the Stratos Library
  printf("Starting Test\n");
  printf("Creating new rect\n");
  sras_rect_t rect = (sras_rect_t){ 1, 1, 1, 1 };
  sras_rect_init(&rect);
  printf("Testing Rect\n");
  printf("x %d, y %d, width %d, height %d\n", sras_rect_get_x(&rect),
                                            sras_rect_get_y(&rect),
                                            sras_rect_get_width(&rect),
                                            sras_rect_get_height(&rect));
  return 0;
}
