# Run a DC motor for a fixed time, then it stops by itself.
# A new command replaces the one that is running.

int16 speed          # -255..255: > 0 forward, < 0 backward, 0 = stop now
uint32 duration_ms   # how long to run (the device also caps this for safety)
