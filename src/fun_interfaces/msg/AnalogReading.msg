# An analog input (slider, distance sensor) moved. Already filtered by the Teensy.
# Published only when the value changes, not continuously.
# Which input it is comes from the topic name, e.g. /teensy/slider_0.

builtin_interfaces/Time stamp   # when the reading arrived
uint16 value                    # 0..1023 (for distance sensors: bigger = closer)
