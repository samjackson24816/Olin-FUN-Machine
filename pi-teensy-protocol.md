# Pi/Teensy Serial Protocol
Version: 1 (buttons only). This file is the single source of truth.
If the firmware and this file disagree, make sure to update

## Transport
- USB serial (Teensy native USB CDC). Baud setting is ignored; use 115200.
- ASCII text, one message per line, terminated by `\n`
- Maximum line length: 63 characters. Longer lines are discarded.
- Fields are separated by single spaces. Commands are uppercase.

## Command example

- READY 1
- BTN 3 1
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
Reordering `BTN_PINS` changes the wire IDs. Add new devices at the end.

We will replicate this pattern for everything

Try to keep the transmissions "clean" whenever possible
Debounce inputs, specify fades or patterns as labeled structures rather than spamming the protocol


## Teensy to Pi messages
| Message | When | Meaning |
|---|---|---|
| `READY 1` | Once after boot/reset | Firmware is up; protocol version 1. The Pi should resend any desired state. |
| `BTN <id> <v>` | On a debounced change | `v` = 1 pressed, 0 released |
| `PONG` | In reply to `PING` | Link is alive |
| `ERR <reason>` | On a bad or unknown line | `reason` is one word, e.g. `unknown` |



## Pi to Teensy messages
| Message | Effect |
|---|---|
| `PING` | Teensy replies `PONG` |
| `SNAPSHOT` | Teensy sends an update of the state of every input, one after another



## Example session