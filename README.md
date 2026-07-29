# Honda 1.5 kW Charger — initial firmware

This repository contains the first, deliberately conservative firmware baseline for
the controller shown in `docs/schematic/Honda_1.5KW_Charger_260729.pdf`.

> **Important:** the schematic uses a **GD32F303RET6**, despite the historical
> repository name. This firmware therefore targets that device. It keeps the PFC,
> LLC and output relays disabled after reset and requires both valid measurements
> and an explicit start request before enabling power conversion.

See [`firmware/README.md`](firmware/README.md) for the verified pin map, build and
bring-up procedure. Never connect the power stage to mains during initial GPIO and
ADC validation.
