# Progress: Nov 16, 2026

Two scenes from [Script.md](../../Script.md) now run end to end in simulation: **Intro** and **Sliders** (with 3 sliders). The Teensy firmware and the Pi/Teensy protocol were extended to support them, following Sam's framework (`firmware/teensy/`, `docs/pi-teensy-protocol.md`).

```
 distance sensor ─┐                                   ┌─► /teensy/led_N, /teensy/strip
 3 sliders ───────┼─► Teensy ──USB──► teensy_node ────┤        (button LEDs, strip)
 4 buttons ───────┘   (firmware)      /teensy/prox_0  │
                                      /teensy/slider_N└─► brain ──► /play_clip ──► clip_player
                                      /teensy/btn_N        (scenes)  /audio/loop    (SIM only so far)
                                                                     /matrix/command
```

## Done

**Teensy firmware** ([firmware/teensy/](../../../firmware/teensy/), built on Sam's skeleton)
- Buttons: filled in the debounce in `updateButtons()` and the `SNAPSHOT` reply.
- Added 3 sliders (A0-A2) and 1 distance sensor (A3). Readings are smoothed and sent only when they change.
- Added button LEDs on the TLC5947: `LED` for a solid colour and `PULSE` for breathing, timed by the Teensy itself so the Pi does not have to keep sending updates.
- Added `STRIP <r> <g> <b>` for the WS2812B strip and `STOP` (everything off).
- Compiles for Teensy 4.1 with no warnings. **Not yet tested on the real board.**

**Protocol** ([pi-teensy-protocol.md](../../pi-teensy-protocol.md)) updated to **version 2**: `SLIDER`, `PROX`, `LED`, `PULSE`, `STRIP`, `STOP`, `OK STOP`, plus an example session. The Teensy now says `READY 2`.

**ROS (Pi side)**
- New messages in `fun_interfaces`: `AnalogReading`, `LedCommand`, `StripCommand`, `SoundLoop`.
- `teensy_node` (fun_hardware): translates between ROS topics and the Teensy's serial lines. It remembers the last command for each output and resends them whenever the Teensy (re)connects or sends `READY`. It has a SIM mode, like `dollar_eater_node`.
- `clip_player_node` (fun_hardware): implements the `PlayClip` action and the `/audio/loop` topic. **SIM only for now**: it logs clips instead of playing them.
- `brain` (fun_core): runs one scene at a time. Scenes use a small `Machine` interface (play a clip, set lights, go to another scene) and never touch ROS directly. A scene that is not written yet logs a warning, and the brain stops there.
- Launch file: `fun_bringup/launch/scenes.launch.xml`.

**Intro scene** (`fun_core/src/scenes/intro_scene.cpp`)
- Nobody around: lights off, big red button dim and solid, an idle line every 15 to 40 s ("Hello?", "ta ta ta", ...).
- Someone nearby: big red button pulses brighter, strip faintly on, call-out lines every 5 to 10 s ("Hey you there!", ...). The first call-out interrupts an idle line.
- The person walks away: back to idle.
- The person comes up to the machine: go to `press_the_button` (not written yet).
- The same line never plays twice in a row. Nearby/close use hysteresis so a wobbly reading does not flicker between levels.

**Sliders scene** (`fun_core/src/scenes/sliders_scene.cpp`)
- Plays the intro line, then gives each of the **3 sliders** a random target at least 350 (out of 1023) from where it is now and at least 80 from either end.
- The first slider move fades in the "noooo" drone. Progress is the average closeness of the three sliders, and it crossfades "noooo" into "yeeessss" with an equal-power curve (constant loudness).
- When all 3 sliders are within ±35 of their targets for 0.5 s: drones stop, the matrix shows `STATUS GOOD`, then the ding, "Self Destruct Sequence Cancelled", and the character's return line. Then the matrix shows `STATUS :(`, the sad idle animation loops, and the brain goes to `reconciliation` (not written yet).

**Hardware test program** ([firmware/teensy_test/](../../../firmware/teensy_test/)) now uses the same command format as Sam's protocol (`MOTOR 0 200 1000`, `LED 2 0 0 255`, `BTN 0 1`, ...).

## Tested

- All ROS packages build in Docker with no warnings.
- 8 unit tests for the scene logic pass (presence levels, slider targets, closeness, crossfade, no repeated lines).
- **Sliders in simulation:** waits for the slider positions, picks targets, fades in the drone, crossfades as the sliders move, and runs the whole ending.
- **Intro in simulation:** idle lines, call-out on arrival (interrupts the idle line), hysteresis keeps "nearby" through a small drop, back to idle when the person leaves, and moves on when they come close.
- **teensy_node hardware mode** over a virtual serial port: connects, sends `SNAPSHOT`, resends LED/strip state after `READY`, publishes `BTN`/`SLIDER`/`PROX`, warns on `ERR` and on unknown lines.
- Teensy firmware and test program compile for Teensy 4.1.
- **Not yet tested on real hardware.**

## How to run

Simulation, on the Mac:

```bash
colima start
cd docker
docker compose --profile sim run --rm sim bash

# inside the container
cd /ws && colcon build && source install/setup.bash
ros2 launch fun_bringup scenes.launch.xml                        # starts with Intro
ros2 launch fun_bringup scenes.launch.xml start_scene:=sliders   # test Sliders alone
```

In a second terminal (`docker exec -it <container-name> bash`, then `source /ws/install/setup.bash`), pretend to be the Teensy:

```bash
# Intro: someone walks up (nearby = 150 or more), then right up to the machine (350 or more)
ros2 topic pub -1 /teensy/sim_line std_msgs/msg/String "{data: 'PROX 0 200'}"
ros2 topic pub -1 /teensy/sim_line std_msgs/msg/String "{data: 'PROX 0 400'}"

# Sliders: give the three positions, read the targets in the log, then move the sliders there
ros2 topic pub -1 /teensy/sim_line std_msgs/msg/String "{data: 'SLIDER 0 100'}"
ros2 topic pub -1 /teensy/sim_line std_msgs/msg/String "{data: 'SLIDER 1 512'}"
ros2 topic pub -1 /teensy/sim_line std_msgs/msg/String "{data: 'SLIDER 2 900'}"
```

Real hardware, on the Pi (Teensy plugged in first):

```bash
ros2 launch fun_bringup scenes.launch.xml use_sim:=false
```

Useful while debugging:

```bash
ros2 topic echo /teensy/prox_0                     # live distance readings, to set the thresholds
ros2 topic echo /teensy/slider_0
ros2 topic echo /audio/loop                        # drone volumes
ros2 topic echo /matrix/command
ros2 run fun_core brain --ros-args -p intro.nearby_threshold:=180 -p intro.close_threshold:=420
```

Without the Pi, the Teensy can be checked alone in the Arduino Serial Monitor (Newline): `PING` → `PONG`, `SNAPSHOT`, `PULSE 0 255 0 0 2000`, `STRIP 0 0 40`, `STOP`.

## Wiring for the new inputs

| Part | Connect to | Note |
|---|---|---|
| Slider 1, 2, 3 wiper | A0, A1, A2 (pins 14, 15, 16) | Slider ends to **3.3V** and GND, never 5V: Teensy 4.1 pins are not 5V tolerant |
| Distance sensor output | A3 (pin 17) | Output must stay below 3.3V. The Sharp GP2Y0A02 / GP2Y0A21 stay under ~3.1V when powered from 5V |
| Big red button LED | LED 0 = TLC5947 OUT0 (R), OUT1 (G), OUT2 (B) | Changed with the `big_red_led` parameter |

## Tuning (no recompiling)

| Parameter | Default | Meaning |
|---|---|---|
| `intro.nearby_threshold` / `intro.close_threshold` | 150 / 350 | Distance readings for "nearby" and "at the machine" (bigger = closer). **Must be measured with the real sensor.** |
| `intro.hysteresis` | 30 | How far a reading must drop below a threshold to leave that level |
| `intro.dim_red` / `intro.pulse_red` / `intro.pulse_period_ms` | 30 / 150 / 2000 | Big red button brightness and breath length |
| `intro.idle_gap_*_sec` / `intro.callout_gap_*_sec` | 15-40 / 5-10 | Pauses between lines |
| `sliders.min_target_distance` / `sliders.edge_margin` | 350 / 80 | How far away targets are picked |
| `sliders.target_margin` / `sliders.hold_sec` | 35 / 0.5 | How close counts as "on target", and for how long |
| `sliders.fade_in_sec` | 1.5 | Drone fade-in after the first move |

## Dollar Eater: where it stands

| Part | Status |
|---|---|
| Arduino firmware: bill switch, `RUN`/`STOP`, 10 s safety cap, `AUTO_EAT` test mode | Done, compiles |
| `dollar_eater_node` + `decision_maker` rule: bill in → motor runs 3 s | Done, tested in simulation and over a virtual serial port |
| Real hardware | **Not tested yet** |
| The rest of the scene: "do you have a dollar?", yes/no buttons, loading animation, red lights, REJECTED, "huh shame" | Not started. It should become a scene in the brain, like Intro and Sliders |

Debugging it, step by step (details in [firmware/dollar_eater/README.md](../../../firmware/dollar_eater/README.md)):
1. **Arduino alone:** Serial Monitor at 115200 → reset shows `READY`; press the switch → `SWITCH 1`; type `RUN 255 2000` → motor turns. Or set `AUTO_EAT = true` to test the mechanism with no computer.
2. **Simulation:** `ros2 launch fun_bringup dollar_eater.launch.xml`, then `ros2 topic pub --once /dollar_eater/sim_switch std_msgs/msg/Bool "{data: true}"`; the log shows `[SIM] would send to Arduino: RUN 255 3000`.
3. **On the Pi:** `ros2 launch fun_bringup dollar_eater.launch.xml use_sim:=false`; watch `ros2 topic echo /dollar_eater/bill_switch`; run the motor directly with `ros2 topic pub --once /dollar_eater/motor fun_interfaces/msg/MotorCommand "{speed: 255, duration_ms: 2000}"`.

## Next

1. Flash `firmware/teensy` and test on the real board; measure the distance sensor to set the Intro thresholds.
2. A real `clip_player` backend (audio + video). It must mix the clip and the looping drones at the same time with live volume (e.g. SDL2_mixer channels).
3. An LED matrix node for `/matrix/command`.
4. The next scenes, starting with **Press the Button** (Intro already hands over to it), and the Dollar Eater scene.

## Open questions for the team

- **Which distance sensor?** The code assumes an analog one (bigger = closer). A PIR sensor only says yes/no and cannot tell "nearby" from "at the machine". An ultrasonic or ToF sensor would need a protocol change.
- **Sam's high-level diagram shows 2 sliders; Script.md needs 3.** The code uses 3.
- **Two USB devices on the Pi:** the Teensy and the Dollar Eater Arduino will both show up as `/dev/ttyACM*`, but `docker/compose.yml` only passes `/dev/ttyACM0`. Either pass both, or move the Dollar Eater motor onto the Teensy as `MOTOR 1` (the Teensy already plans 4 DRV8871s).
- The intermediate build drives button LEDs with a **TLC5947**; the high-level diagram says **PCA9685 ×2**. The firmware uses the TLC5947 for now.
