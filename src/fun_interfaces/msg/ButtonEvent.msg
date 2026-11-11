# A button or switch changed state (already debounced).
# Published only when the state CHANGES, not continuously.
# Which button it is comes from the topic name, e.g. /yes_button/pressed.

builtin_interfaces/Time stamp   # when the change happened
bool pressed                    # true = pressed down, false = released
