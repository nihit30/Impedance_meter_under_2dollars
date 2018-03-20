# Impedance-meter-under-2-

HARDWARE TARGET : TM4C123GH6PM

Refer LT spice simulation for reference while reading code

Pins MEAS_LR, LOWSIDE_R and INTEGRATE are used for measuring resistance and inductor only, using these pins RC and RL circuit
paths can be created through device connected at DUT terminal.

Using time constant formula, we can easily calculate value of device connected

Pins MEAS_C and HIGHSIDE_R are used to measure capacitance only.

