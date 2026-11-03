# Reverse engineering notes — Trust GXT 872 Xyra

Everything learned while building this tool. Keep this file updated when
new features are decoded.

## Device

- USB `145f:0367` ("BY Tech GXT 872 Xyra"), full-speed.
- Interface 0: boot keyboard, EP `0x81`.
- Interface 1: vendor HID, usage page `0xFF00`, EP `0x82` IN (16 B).
- Feature reports on interface 1:
  - report 5: 5 bytes, **write-only** (GET stalls).
  - report 6: 519 data bytes (520 with report ID).

## Working commands

### Colour — report 6, command `0x0a`

```
[0]=0x06  [1]=0x0a  [4]=0x01  [7]=0x02  [29..31]=R,G,B
```

Sending only this report changes the whole main zone to a solid colour.
Bytes 32.. are **not** the per-key buffer (filling them changes nothing on
the main keys; only the left side block reacts, which is a separate zone).

### Configuration — report 6, command `0x04`

Fixed 520-byte template (captured from the vendor app, see
`src/core/gxt872.c`), variable bytes:

| Offset | Meaning | Values |
|---|---|---|
| 18 | effect | see table |
| 66 | brightness | 0..3 |
| 69 | speed | `0x07 + 0x10 * level`, level 0..4 |

Effect IDs: `01` static, `02` breathing, `03` rainbow cycle, `04`
horizontal ripple, `05` raindrops, `07` ripple burst, `08` sparkle, `0a`
snake, `0b` rainbow wave, `0c` reactive, `0d` sine wave, `0f` rainbow
wheel, `10` rainbow waterfall, `11` bloom, `15` custom mode.

Note: the template was originally transcribed truncated (130 instead of
520 bytes), which silently broke colour/effect/brightness. Always take the
template from a real capture.

## Open: per-key colours and secondary zones

- `0x15` puts the keyboard in the vendor "custom" mode. The app then sends
  a companion **report 6, command `0x06`** packet plus `0x84` and `0x04`.
- A `0x06` packet captured from the app (see `cap8.pcap` in the session,
  or re-capture) had non-zero bytes only at: 8, 134, 143, 260, 269, 274,
  275, 281, 349, 354, 355, 361.
- Hypothesis: `byte 8 + 3*i` = R of key *i* (87 keys → 8..268), but:
  - writing a minimal `0x06` with only key 0 red appeared to light Esc;
  - writing key 1 alone lights nothing (per the owner's eyes);
  - writing indices 44/45/46 lights nothing.
  So either the packet needs the constant bytes (269 / 349 blocks) or the
  table starts elsewhere / is not 3 bytes per key.
- Side block (Esc/arrows/printscreen group) and the front light bar are
  separate zones: they do not follow the main colour, the main brightness
  (bar stays on at brightness 0) and are not controlled by the `0x0a`
  buffer. Likely firmware-only (Fn combinations), i.e. no USB traffic.

## Next steps

1. Re-capture the app while setting **two different keys to two different
   colours**, saving each time, and diff the `0x06` packets byte by byte.
   The key indices will appear as the differing bytes.
2. Check whether the constant tail (offset 269+) is a required checksum or
   a per-zone table.
3. Once the index→key mapping is known, add `--key NAME=RRGGBB` to the CLI
   and a per-key editor to the GUI.

## Capture recipe (used for everything above)

```sh
# vendor app under Wine (32-bit prefix, needs wine-core.i686)
sudo tcpdump -i usbmon3 -U -s 0 -w cap.pcap
# ... interact with the app ...
# parse SET_REPORT feature packets
python3 - <<'PY'
# see the scripts used in the session; filter URB type 'S', bmRequestType 0x21,
# bRequest 0x09, wValue 0x0306
PY
```
