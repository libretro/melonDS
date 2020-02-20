#include <algorithm>

#include "utils.h"

int32_t Clamp(int32_t value, int32_t min, int32_t max)
{
   return std::max(min, std::min(max, value));
}

void copy_screen(ScreenLayoutData *data, uint32_t* src, unsigned offset)
{
   if (data->direct_copy)
   {
      memcpy((uint32_t *)data->buffer_ptr + offset, src, data->screen_width * data->screen_height * data->pixel_size);
   } else {
      unsigned y;
      for (y = 0; y < data->screen_height; y++)
      {
         memcpy((uint16_t *)data->buffer_ptr + offset + (y * data->screen_width * data->pixel_size),
            src + (y * data->screen_width), data->screen_width * data->pixel_size);
      }
   }
}

void draw_cursor(ScreenLayoutData *data, int32_t x, int32_t y)
{
   uint32_t* base_offset = (uint32_t*)data->buffer_ptr;

   uint32_t start_y = Clamp(y - CURSOR_SIZE, 0, data->screen_height);
   uint32_t end_y = Clamp(y + CURSOR_SIZE, 0, data->screen_height);

   for (uint32_t y = start_y; y < end_y; y++)
   {
      uint32_t start_x = Clamp(x - CURSOR_SIZE, 0, data->screen_width);
      uint32_t end_x = Clamp(x + CURSOR_SIZE, 0, data->screen_width);

      for (uint32_t x = start_x; x < end_x; x++)
      {
         uint32_t* offset = base_offset + ((y + data->touch_offset_y) * data->buffer_width) + ((x + data->touch_offset_x));
         uint32_t pixel = *offset;
         *(uint32_t*)offset = (0xFFFFFF - pixel) | 0xFF000000;
      }
   }
}
