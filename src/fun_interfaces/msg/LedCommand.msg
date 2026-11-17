# What one button LED should do. Which LED comes from the topic name, e.g. /teensy/led_0.
# The LED keeps doing this until the next command. Solid 0 0 0 = off.

# Possible values for `mode`
uint8 MODE_SOLID=0   # stay at the colour
uint8 MODE_PULSE=1   # breathe from off to the colour and back, once per period_ms

uint8 mode
uint8 r              # 0..255
uint8 g
uint8 b
uint16 period_ms     # used when mode == MODE_PULSE (100..60000)
