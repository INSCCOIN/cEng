# cEng

A one-cylinder 4-stroke you can see and (if `aplay` is there) hear.

Inspired by Ange Yaghi’s Engine Simulator. This is **not** a port of that tree. It is a small C slider-crank on `/dev/fb0` for the SharkDeck. Engine Simulator is MIT; this work is separate.

## Build on the deck

```bash
make
./cEng
```
Do note, if you're on an ARM based CPU, you'll need to remove all o files.

```
rm -f *.o cEng
```

Rebuild from `.c` files on the device. Do not copy `.o` files from another CPU.

Needs `/dev/fb0`. Sound is optional: `aplay` raw s16le 22050 Hz. No `aplay` = silent picture.

Log: `/home/ceng.log` (wiped on launch).

## Keys

| | |
|--|--|
| `W` `S` | throttle |
| Space (hold) | starter |
| `I` | ignition on/off |
| `R` | reset |
| `Q` | quit |

Crank it with space until it fires, then feather throttle. Cut `I` and it dies. Mash `W` and the flywheel will scream.

## What is actually simulated

- Slider-crank geometry (rod + crank, finite length)
- Chamber volume from piston position
- `PV^γ` compression / expansion
- Intake and exhaust valves as pressure leaks
- Spark near TDC if mix and pressure are enough
- Flywheel inertia, friction, starter torque
- Side-view piston, rod, crank, gas color from pressure / fire

It is **not** a dyno, not CFD, and not Ange’s fluid audio. It is a working four-stroke cartoon that idles if you treat it like an engine.
