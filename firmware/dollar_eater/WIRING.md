# Dollar Eater Wiring

| From | To |
|---|---|
| Switch wire 1 | Arduino **D2** |
| Switch wire 2 | Arduino **GND** |
| DRV8871 **IN1** | Arduino **D9** |
| DRV8871 **IN2** | Arduino **D10** |
| DRV8871 **GND** | Arduino **GND** (grounds must be connected) |
| Motor wires (2) | DRV8871 **OUT1** / **OUT2** |
| Motor power supply + / − | DRV8871 power terminal |

## Notes

- **Motor power:** the DRV8871 needs its own supply of **6.5–45 V**, matching the motor's rated voltage. The Arduino's 5 V USB power cannot run it.
- **Common ground:** the DRV8871 GND must be connected to the Arduino GND, or the motor will not respond.
- **USB cable:** the Arduino connects to the computer or Pi with USB, which powers it and carries the serial messages.
- **Switch:** no resistor needed; the Arduino's internal pull-up is used.
- **Motor turns the wrong way** (pushes the bill out): swap the two motor wires.
