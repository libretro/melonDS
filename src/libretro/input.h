#ifndef _INPUT_H
#define _INPUT_H

#include "types.h"
#include <libretro.h>

enum TouchMode
{
   Disabled,
   Mouse,
   Touch,
   Joystick,
};

enum MicInputMode
{
    BlowNoise,
    WhiteNoise,
    MicInput,
};

struct InputState
{
   bool touching;
   int touch_x, touch_y;
   TouchMode current_touch_mode;

   bool previous_holding_noise_btn = false;
   bool holding_noise_btn = false;
   bool swap_screens_btn = false;
   bool lid_closed = false;
};

extern InputState input_state;

bool cursor_enabled(InputState *state);

extern bool libretro_supports_bitmasks;
extern bool noise_button_required;
extern retro_microphone_interface micInterface;
extern retro_microphone_t *micHandle;
extern MicInputMode micNoiseType;

void update_input(InputState *state);

#endif
