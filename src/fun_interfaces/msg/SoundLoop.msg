# Set the volume of a looping sound (e.g. the "noooo" and "yeesss" drones in the Sliders scene).
# Several loops can play at once, on top of the clip from the PlayClip action.
#
# Rules for the player:
#  - volume > 0 and the loop is not playing: start looping it at that volume.
#  - volume > 0 and it is playing: change the volume (no restart).
#  - volume == 0: stop it.

string clip_id   # e.g. "sliders_nooo", mapped to a file like PlayClip
float32 volume   # 0.0 .. 1.0
