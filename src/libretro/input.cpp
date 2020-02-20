#include "input.h"
#include "libretro_state.h"
#include "utils.h"

#include "NDS.h"

InputState input_state;

void update_input(InputState *state)
{
   input_poll_cb();

   uint16_t keys = 0;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) << 0;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)) << 1;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT)) << 2;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START)) << 3;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) << 4;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) << 5;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)) << 6;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN)) << 7;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)) << 8;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L)) << 9;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X)) << 10;
   keys |= (!!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)) << 11;

   for (uint8_t i = 0; i < 12; i++) {
      bool key = !!((keys >> i) & 1);
      uint8_t nds_key = i > 9 ? i + 6 : i;


      if (key) {
         NDS::PressKey(nds_key);
      } else {
         NDS::ReleaseKey(nds_key);
      }
   }


   bool lid_closed_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);
   if(lid_closed_btn != state->lid_closed)
   {
      NDS::SetLidClosed(lid_closed_btn);
      state->lid_closed = lid_closed_btn;
   }

   state->holding_noise_btn = !!input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);

   if(current_screen_layout != ScreenLayout::TopOnly)
   {
      switch(state->current_touch_mode)
      {
         case TouchMode::Disabled:
            state->touching = false;
            break;
         case TouchMode::Mouse:
            {
               int16_t mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
               int16_t mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

               state->touching = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);

               state->touch_x = Clamp(state->touch_x + mouse_x, 0, VIDEO_WIDTH);
               state->touch_y = Clamp(state->touch_y + mouse_y, 0, VIDEO_HEIGHT);
            }

            break;
         case TouchMode::Touch:
            if(input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED))
            {
               int16_t pointer_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
               int16_t pointer_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

               unsigned int x = ((int)pointer_x + 0x8000) * screen_layout_data.buffer_width / 0x10000;
               unsigned int y = ((int)pointer_y + 0x8000) * screen_layout_data.buffer_height / 0x10000;

               if ((x >= screen_layout_data.touch_offset_x) && (x < screen_layout_data.touch_offset_x + screen_layout_data.screen_width) &&
                     (y >= screen_layout_data.touch_offset_y) && (y < screen_layout_data.touch_offset_y + screen_layout_data.screen_height))
               {
                  state->touching = true;

                  state->touch_x = (x - screen_layout_data.touch_offset_x) * VIDEO_WIDTH / screen_layout_data.screen_width;
                  state->touch_y = (y - screen_layout_data.touch_offset_y) * VIDEO_HEIGHT / screen_layout_data.screen_height;
               }
            }
            else if(state->touching)
            {
               state->touching = false;
            }

            break;
      }
   }
   else
   {
      state->touching = false;
   }

   if(state->touching)
   {
      NDS::TouchScreen(state->touch_x, state->touch_y);
      NDS::PressKey(16+6);
   }
   else
   {
      NDS::ReleaseScreen();
      NDS::ReleaseKey(16+6);
   }
}
