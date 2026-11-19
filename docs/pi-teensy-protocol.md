# Pi/Teensy Serial Protocol
Version: 2 (buttons, sliders, proximity, button LEDs, strip). This file is the single source of truth.
If the firmware and this file disagree, make sure to update

## Transport
- USB serial (Teensy native USB CDC). Baud setting is ignored; use 115200.
- ASCII text, one message per line, terminated by `\n`
- Maximum line length: 63 characters. Longer lines are discarded.
- Fields are separated by single spaces. Commands are uppercase.

## Command example

- READY 2
- BTN 3 1
- SLIDER 0 512
- PULSE 0 255 0 0 2000
- PING

## Device IDs
- IDs are 0-based integers, per device type.
- The ID is the index into the matching array in `pins.h`.
- The Teensy knows IDs only. Names ("start button") live in Pi config.


### Example
| Type | ID | Schematic label | Teensy pin |
|---|---|---|---|
| BTN | 0 | Btn 1 | 2 |
| BTN | 1 | Btn 2 | 3 |
| BTN | 2 | Btn 3 | 4 |
| BTN | 3 | Btn 4 | 5 |
| SLIDER | 0 | Slider 1 | A0 (14) |
| SLIDER | 1 | Slider 2 | A1 (15) |
| SLIDER | 2 | Slider 3 | A2 (16) |
| PROX | 0 | Distance sensor | not wired yet (A3 / pin 17 is now the Start button) |
| LED | 0-3 | Button RGB LEDs | TLC5947 OUT0-11 (3 outputs each, R G B) |
| STRIP | - | WS2812B strip | 33 |
Reordering `BTN_PINS` changes the wire IDs. Add new devices at the end.

We will replicate this pattern for everything

Try to keep the transmissions "clean" whenever possible
Debounce inputs, specify fades or patterns as labeled structures rather than spamming the protocol


## Teensy to Pi messages
| Message | When | Meaning |
|---|---|---|
| `READY 2` | Once after boot/reset | Firmware is up; protocol version 2. All outputs are off. The Pi should resend any desired state. |
| `BTN <id> <v>` | On a debounced change | `v` = 1 pressed, 0 released |
| `SLIDER <id> <v>` | When the value moves more than 8 since the last report, at most every 20 ms | `v` = 0..1023 |
| `PROX <id> <v>` | When the value moves more than 10 since the last report, at most every 50 ms | `v` = 0..1023, raw distance reading; bigger = closer |
| `PONG` | In reply to `PING` | Link is alive |
| `OK STOP` | In reply to `STOP` | Everything is off |
| `ERR <reason>` | On a bad or unknown line | `reason` is one word: `unknown`, `range`, `toolong` |

`LED`, `PULSE` and `STRIP` get no reply when they work (they are state commands, sending one twice is the same as once). They reply `ERR range` if a value is out of range.


## Pi to Teensy messages
| Message | Effect |
|---|---|
| `PING` | Teensy replies `PONG` |
| `SNAPSHOT` | Teensy sends an update of the state of every input, one after another (`BTN`, then `SLIDER`, then `PROX`) |
| `LED <id> <r> <g> <b>` | Button LED to a solid colour, 0..255 each. `LED <id> 0 0 0` = off |
| `PULSE <id> <r> <g> <b> <period_ms>` | Button LED breathes from off to the colour and back, once per `period_ms` (100..60000), until the next `LED`/`PULSE` for that ID |
| `STRIP <r> <g> <b>` | Whole strip to one colour, 0..255 each. `STRIP 0 0 0` = off |
| `STOP` | Every output off. Teensy replies `OK STOP` |



## Example session
```
Teensy: READY 2
Pi:     SNAPSHOT
Teensy: BTN 0 0
Teensy: BTN 1 0
Teensy: BTN 2 0
Teensy: BTN 3 0
Teensy: SLIDER 0 104
Teensy: SLIDER 1 512
Teensy: SLIDER 2 930
Teensy: PROX 0 87
Pi:     LED 0 30 0 0             (big red button, dim)
Teensy: PROX 0 260               (someone walks up)
Pi:     PULSE 0 150 0 0 2000     (big red button starts breathing)
Teensy: SLIDER 1 530
Pi:     STOP
Teensy: OK STOP
```
