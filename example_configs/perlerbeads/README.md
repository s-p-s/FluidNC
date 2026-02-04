# Perler beads + TCS3472 samples

This folder contains example job files (`.nc`) that work with the `tcs3472` module added to FluidNC.

## Requirements

- Your YAML config must define an I2C bus section (e.g. `i2c0:`).
- Your YAML config must define a `tcs3472:` instance with an `id:` (these samples assume `id: 0`).

Example snippet:

```yaml
i2c0:
  sda_pin: gpio.22:pu
  scl_pin: gpio.15:pu

tcs3472:
  id: 0
  i2c_num: 0
  i2c_address: 0x29
  integration_ms: 50
  gain: 16
```

## Commands used

- `$TCS=LIST`
- `$TCS=READ id=0 fresh=yes p=tcs`
- `$TCS=LAST id=0 p=tcs`

The `p=` parameter sets the named-parameter prefix. With `p=tcs`, the firmware writes:

- `#<TCS_C> #<TCS_R> #<TCS_G> #<TCS_B>`
- `#<TCS_RN> #<TCS_GN> #<TCS_BN>` (normalized by Clear, 0..1)
