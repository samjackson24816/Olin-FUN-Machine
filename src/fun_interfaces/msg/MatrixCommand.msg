# What the LED matrix should show.

# Possible values for `mode`
uint8 MODE_OFF=0
uint8 MODE_COUNTDOWN=1   # count down until countdown_end
uint8 MODE_TEXT=2        # show `text`, e.g. "REJECTED"

uint8 mode
string text                              # used when mode == MODE_TEXT
builtin_interfaces/Time countdown_end    # used when mode == MODE_COUNTDOWN
