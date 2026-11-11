# Dollar Eater

Push a bill into the slot → it presses the switch → the motor pulls the bill in.

**Hardware:** Arduino Uno R3, DRV8871 motor driver, one DC motor, one mechanical switch.

## How it works

```
switch ──► Arduino ──USB──► dollar_eater_node ──/dollar_eater/bill_switch──► decision_maker
                                                                                  │ "eat it"
motor ◄── DRV8871 ◄── Arduino ◄──USB── dollar_eater_node ◄──/dollar_eater/motor───┘
```

- **Arduino** only reports the switch and runs the motor when told. It never decides.
- **dollar_eater_node** (on the Pi) passes messages between the Arduino and ROS.
- **decision_maker** decides: bill inserted → run the motor at full speed for 3 seconds.

| File | What it is |
|---|---|
| `firmware/dollar_eater/dollar_eater.ino` | Arduino code |
| `src/fun_hardware/src/dollar_eater_node.cpp` | ROS node that talks to the Arduino |
| `src/fun_hardware/src/serial_port.cpp` | Small USB-serial helper |
| `src/fun_core/src/decision_maker.cpp` | The "bill inserted → eat" rule |
| `src/fun_interfaces/msg/MotorCommand.msg` | Message: motor speed + how long |
| `src/fun_bringup/launch/dollar_eater.launch.xml` | Starts everything with one command |

## 1. Wiring

| From | To |
|---|---|
| Switch wire 1 | Arduino **D2** |
| Switch wire 2 | Arduino **GND** |
| DRV8871 **IN1** | Arduino **D9** |
| DRV8871 **IN2** | Arduino **D10** |
| DRV8871 **GND** | Arduino **GND** (grounds must be connected) |
| Motor wire 1 / wire 2 | DRV8871 **OUT1** / **OUT2** |
| Motor power supply + / − | DRV8871 power input (**VM** / **GND**) |

- The DRV8871 needs a **separate motor power supply of 6.5–45 V** that matches your motor. USB power from the Arduino cannot run the motor.
- The switch needs no resistor (the Arduino's internal pull-up is used).
- Motor turns the wrong way (pushes the bill out)? Swap the two motor wires.

## 2. Upload the Arduino code

1. Open `firmware/dollar_eater/dollar_eater.ino` in the Arduino IDE.
2. **Tools → Board → Arduino Uno**, and pick the port.
3. Click **Upload**.

## 3. Test the Arduino alone (no Pi, no ROS)

Open **Tools → Serial Monitor**, set **115200 baud** and **Newline**.

| Do this | You should see |
|---|---|
| Press the reset button | `READY` |
| Press the switch by hand, then let go | `SWITCH 1`, then `SWITCH 0` |
| Type `RUN 255 2000` | Motor turns for 2 s, then `DONE` |
| Type `RUN -255 1000` | Motor turns the other way for 1 s |
| Type `STOP` | Motor stops right away |

**Eat by itself, with no computer involved:** set `AUTO_EAT = true` at the top of the `.ino`, upload, then insert a bill: the motor runs for 3 s. This is handy for testing the mechanism. **Set it back to `false` before using the Pi.**

Close the Serial Monitor before starting ROS. Only one program can use the port at a time.

## 4. Run in simulation on the Mac (no hardware)

Docker on the Mac cannot see USB devices, so on the Mac the switch is faked with a topic.

```bash
colima start
cd docker
docker compose --profile sim run --rm sim bash

# inside the container
cd /ws && colcon build && source install/setup.bash
ros2 launch fun_bringup dollar_eater.launch.xml
```

In a second terminal, fake a bill:

```bash
docker ps                          # find the container name
docker exec -it <container-name> bash
source /opt/ros/lyrical/setup.bash && source /ws/install/setup.bash
ros2 topic pub --once /dollar_eater/sim_switch std_msgs/msg/Bool "{data: true}"
```

The first terminal should show:

```
[dollar_eater]: Bill switch PRESSED
[decision_maker]: Bill inserted, eating it (speed 255 for 3000 ms)
[dollar_eater]: [SIM] would send to Arduino: RUN 255 3000
```

## 5. Run on the Raspberry Pi (real hardware)

1. Plug the Arduino into the Pi's USB **before** starting Docker.
2. Check its name: `ls /dev/ttyACM* /dev/ttyUSB*`. A genuine Uno is `/dev/ttyACM0`. A clone board is often `/dev/ttyUSB0`; in that case, change the `devices:` line in `docker/compose.yml` and add `port:=/dev/ttyUSB0` to the launch command.
3. Start it:

```bash
cd docker
docker compose --profile hw run --rm hw bash

# inside the container
cd /ws && colcon build && source install/setup.bash
ros2 launch fun_bringup dollar_eater.launch.xml use_sim:=false
```

You should see `Connected to /dev/ttyACM0`, then `Arduino: READY` about 2 seconds later (the Arduino restarts whenever the port is opened). Insert a bill: the motor eats it.

## Tuning

Change these at launch, no recompiling needed:

```bash
ros2 launch fun_bringup dollar_eater.launch.xml use_sim:=false eat_speed:=200 eat_ms:=2500
```

| Setting | Default | Meaning |
|---|---|---|
| `eat_speed` | `255` | Motor speed, -255..255 (negative = other direction) |
| `eat_ms` | `3000` | How long the motor runs, in ms |

In the `.ino`: pins, `DEBOUNCE_MS` (20), `MAX_RUN_MS` (10000, a safety limit on any single run), and `PRESSED_LEVEL`.

## Serial protocol (Pi ↔ Arduino)

115200 baud, one text message per line.

| Direction | Message | Meaning |
|---|---|---|
| Pi → Arduino | `RUN <speed> <ms>` | Run at speed -255..255 for ms, then stop |
| Pi → Arduino | `STOP` | Stop now |
| Arduino → Pi | `READY` | Finished starting up |
| Arduino → Pi | `SWITCH 1` / `SWITCH 0` | Switch pressed / released |
| Arduino → Pi | `OK <command>` | Command accepted |
| Arduino → Pi | `DONE` | Timed run finished |
| Arduino → Pi | `ERR <text>` | Command not understood |

## Troubleshooting

| Problem | Fix |
|---|---|
| `Waiting for Arduino on /dev/ttyACM0 ...` | Not plugged in, wrong port name (see step 5), or the Serial Monitor is still open |
| Docker: `error gathering device information` | Plug the Arduino in before starting the container |
| `SWITCH 1` shows, but the motor does not move | Motor power supply off, or the DRV8871 GND is not connected to the Arduino GND. Try `RUN 255 2000` in the Serial Monitor |
| Motor turns the wrong way | Swap the two motor wires, or use `eat_speed:=-255` |
| `SWITCH 1` when nothing is pressed (backwards) | The switch is normally-closed: set `PRESSED_LEVEL = HIGH` |
| Several `SWITCH 1` for one bill | Increase `DEBOUNCE_MS` (try 50) |

## Next

The "bill inserted → eat" rule is the first step. In the full script, the Dollar Eater section also plays voice lines, turns the lights red and shows REJECTED. Those will be added to the decision maker later.
