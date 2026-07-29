# Initial firmware and commissioning notes

## Scope and safety state

The schematic identifies U17 as `GD32F303RET6`. The implementation is a small
register-level baseline so it does not depend on an unavailable vendor SDK. It
provides a testable power-stage state machine, open-loop CV/current PWM commands,
soft start, relay sequencing, fault latching and status LEDs.

The checked-in image is intentionally **commissioning locked**:
`sample_inputs()` reports `hardware_fault = true`. Consequently it cannot enable
the PFC relay or LLC stage. Replace that hook only after ADC channel calibration,
active-level checks and low-voltage fault-injection tests have passed.

## Schematic pin map

| Function | U17 pin | GPIO |
|---|---:|---|
| AC voltage | 15 | PA1/ADC |
| Fan current | 16 | PA2/ADC |
| Bus voltage | 17 | PA3/ADC |
| Case temperature | 20 | PA4/ADC |
| Output voltage | 21 | PA5/ADC |
| Current | 22 | PA6/ADC |
| PFC MOS temperature | 23 | PA7/ADC |
| Transformer temperature | 24 | PC4/ADC |
| Battery voltage | 25 | PC5/ADC |
| Bus-voltage adjust | 26 | PB0/ADC |
| LLC MOS temperature | 27 | PB1/ADC |
| LLC fault check | 33 | PB12 |
| Current command PWM | 34 | PB13 |
| Hardware-fault clear | 35 | PB14 |
| LLC enable | 36 | PB15 |
| Commissioning input TEST1 | 37 | PC6 |
| Red / green LEDs | 39 / 40 | PC8 / PC9 |
| CV command PWM | 41 | PA8 |
| Output relay | 42 | PA9 |
| CAN RX / TX | 44 / 45 | PA11 / PA12 |
| PFC relay/enable | 51 | PC10 |
| Fan control | 53 | PC12 |

## Build and bring-up

```sh
make -C firmware host-test
make -C firmware                 # needs arm-none-eabi-gcc/binutils
```

Recommended sequence:

1. Keep mains and the high-voltage DC bus disconnected; verify every output's
   inactive level and PWM pin on an oscilloscope.
2. Implement ADC acquisition and board-specific calibration in `sample_inputs()`.
3. Inject each over-current, over-voltage, thermal and LLC hardware fault and
   confirm all relays and `LLC_EN` drop immediately.
4. Use an isolated low-voltage DC source and current-limited load to tune the
   command direction and soft-start values.
5. Only then commission the PFC and LLC at high voltage under an approved lab
   safety procedure. The limits in `charger_config.h` are placeholders, not
   production calibration values.

The requested `llc_open_loop` branch was not present in the supplied Git object
database, so no code could be copied or compared from it. This implementation is
derived from the supplied schematic's U17 pin/net labels and should be reconciled
with that branch when it becomes available.
