Arduino sketch for controlling CSR8645 BT module, 
which analyzes the voltage at the analog input (Mazda 3 BK Steering Wheel Button Detection)
and controls the digital outputs according to the set conditions.

Conditions:
- If voltage 3.70-3.90V is >3000ms -> enable RESET_PIN (reset CSR8645 BT module).
- If voltage 2.30-2.50V is >300ms  -> enable PREV_PIN (PREV button of CSR8645 BT module).
- If voltage 1.50-1.80V is >300ms  -> turn on NEXT_PIN (button NEXT CSR8645 BT module).
- At system startup a 3000ms pulse is applied to RESET_PIN (reset CSR8645 BT module).

Measurements (DEBUG 1, J701 ST-SW1 white with black stripe):
- No button pressed = 4.38V
- Pressed Mute = 3.80V
- Pressed Next = 1.67V
- Pressed Prev = 2.40V
- Pressed Vol+ = 0.98V
- Pressed Vol- = 0.41V
- Pressed Mode = 3.13V
