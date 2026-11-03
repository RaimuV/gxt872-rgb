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
| Solid colour on all keys | ✅ working |
| Turn the backlight off | ✅ working |
| Device auto-detection | ✅ working |
| CLI + GUI | ✅ working |
| Brightness | 🚧 not mapped yet |
| Effects (breathing, rainbow, ...) | 🚧 not mapped yet |
| Per-key colours | 🚧 not mapped yet |

## How it works

The keyboard's vendor HID interface (usage page `0xFF00`, interface 1,
`wMaxPacketSize` 16) accepts **feature report 6 with 520 bytes**:

```
report[0]      = 0x06      report ID
report[1]      = 0x0a      command: set LED colour
report[4]      = 0x01
report[7]      = 0x02
report[29..31] = R, G, B
report[32..519]= 0x00
```

Sending this single report turns the whole keyboard to a solid colour.
The protocol was reverse engineered by capturing the vendor software
(USBPcap/usbmon) while changing colours; the colour bytes were found by
diffing a red and a green capture.

The same protocol also works through the 2.4 GHz dongle, if your model has
one.

## Requirements

- Linux
- `hidraw` (kernel built-in)
- for the GUI: Qt 6 Widgets

Tested on **Fedora 44** with kernel `7.2.13` and Qt `6.11`.

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
gxt872-rgb --list                 # show matching hidraw nodes
gxt872-rgb --color red            # named colours
gxt872-rgb --color 00FF80         # hex
gxt872-rgb --color '#FF00FF'      # hex with #
gxt872-rgb --off                  # turn the backlight off
gxt872-rgb --device /dev/hidraw8 --color blue
```

Named colours: `red green blue white yellow magenta cyan orange purple pink`.

## GUI

```sh
gxt872-rgb-gui
```

Colour preview (click to open the colour picker), R/G/B sliders, **Applica**
and **Spegni** buttons. The GUI uses the same core library as the CLI.

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

`tools/probe.c` was used to dump feature reports and to try writes during
the investigation. Keep it around: it is the tool to extend when mapping
brightness, effects and per-key colours.

## License

MIT — see [LICENSE](LICENSE).
