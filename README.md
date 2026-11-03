# gxt872-rgb

Linux control for the RGB lighting of the **Trust GXT 872 Xyra** TKL
mechanical keyboard (`145f:0367`), with a small CLI and a Qt 6 GUI.

No proprietary software, no Wine, no root: the keyboard exposes a standard
HID feature report and this project speaks it directly through
`/dev/hidraw`.

- `gxt872-rgb` — command line frontend
- `gxt872-rgb-gui` — Qt 6 GUI
- `libgxt872` — core library (C, no UI dependencies), reusable by other
  frontends

## Status

| Feature | Status |
|---|---|
| Solid colour | ✅ working |
| Effects (15 effects) | ✅ working |
| Brightness (0..3) | ✅ working |
| Animation speed (0..4) | ✅ working |
| Turn the backlight off | ✅ working |
| Device auto-detection | ✅ working |
| CLI + GUI | ✅ working |
| Per-key custom colours | 🚧 not mapped yet |

## The protocol

Two commands, sent as **feature report 6 with 520 bytes** on the vendor
interface (usage page `0xFF00`, interface 1):

### Colour — command `0x0a`

```
report[0]      = 0x06      report ID
report[1]      = 0x0a      command
report[4]      = 0x01
report[7]      = 0x02
report[29..31] = R, G, B
rest           = 0x00
```

### Configuration — command `0x04`

A fixed device configuration block with three variable bytes:

| Offset | Field | Values |
|---|---|---|
| 18 | effect | see table below |
| 66 | brightness | 0..3 |
| 69 | speed | `0x07 + 0x10 * level`, level 0..4 |

All the other bytes are constant device configuration (zone layout,
checksum `0x5aa5` at offset 104). The template is in
`src/core/gxt872.c`.

### Effect IDs

| Effect | ID | Effect | ID |
|---|---|---|---|
| Static | `0x01` | Snake | `0x0a` |
| Breathing | `0x02` | Rainbow Wave | `0x0b` |
| Rainbow Cycle | `0x03` | Reactive | `0x0c` |
| Horizontal Ripple | `0x04` | Sine Wave | `0x0d` |
| Raindrops | `0x05` | Rainbow Wheel | `0x0f` |
| Ripple Burst | `0x07` | Rainbow Waterfall | `0x10` |
| Sparkle | `0x08` | Bloom | `0x11` |
| | | Off | `0x15` |

The protocol was reverse engineered by capturing the vendor software: the
Windows application was run under **Wine** on Linux and its USB traffic was
captured with `usbmon`/`tcpdump`; the colour bytes were found by diffing a
red and a green capture, the effect IDs by clicking every effect once.

## Requirements

- Linux
- `hidraw` (kernel built-in)
- for the GUI: Qt 6 Widgets

Tested on **Fedora 44** with kernel `7.2.13` and Qt `6.11`, on a GXT 872
Xyra with firmware `4.06`.

## Build

```sh
cmake -S . -B build
cmake --build build -j
```

Binaries end up in `build/gxt872-rgb` and `build/gxt872-rgb-gui`.

## Permissions (udev)

By default `/dev/hidraw*` is root-only. Install the provided rule so the
logged-in user can access the keyboard without `sudo`:

```sh
sudo cp data/70-gxt872-rgb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=hidraw
```

Then unplug and replug the keyboard once.

## Usage

```sh
gxt872-rgb --list                          # show matching hidraw nodes

gxt872-rgb --color red                     # named colours
gxt872-rgb --color 00FF80                  # hex
gxt872-rgb --color '#FF00FF'               # hex with #

gxt872-rgb --effect breathing              # select an effect
gxt872-rgb --brightness 3                  # 0..3
gxt872-rgb --speed 4                       # 0..4

gxt872-rgb --off                           # backlight off

# combine them (applied in this order)
gxt872-rgb --color 00FF80 --effect breathing --brightness 2 --speed 3

gxt872-rgb --device /dev/hidraw8 --color blue
```

Named colours: `red green blue white yellow magenta cyan orange purple pink`.

## GUI

```sh
gxt872-rgb-gui
```

Effect selector, colour preview (click to open the colour picker), R/G/B
sliders, brightness and speed sliders, **Applica** and **Spegni**. The GUI
uses the same core library as the CLI.

## Install

```sh
sudo cmake --install build
```

installs both binaries and the desktop entry. The udev rule is documented
above and intentionally not installed automatically.

## Repository layout

```
src/core/   gxt872.c, gxt872.h     protocol + discovery (C, no UI)
src/cli/    main.c                 command line frontend
src/gui/    main.cpp, mainwindow.* Qt 6 frontend
data/       udev rule, desktop entry
tools/      probe.c                diagnostics used while reverse engineering
```

## Reverse engineering notes

`tools/probe.c` dumps feature reports and tries raw writes; it is the tool
to extend if you want to map the remaining features (per-key colours,
secondary zone, top light bar).

## License

MIT — see [LICENSE](LICENSE).
