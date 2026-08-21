# travelers-board-split Bootloader Decision Record

> **Synchronization rule:** Keep this document and `BOOTLOADER_jp.md` in sync.
> `BOOTLOADER.md` is the English handoff copy for the ZMK firmware repository.

## Scope and decisions

This is the bootloader-specific variant of `travelers-board-split`. The
existing ZMK direct-boot definition remains unchanged as the comparison build
and the initial fallback path.

| Topic | Decision |
| --- | --- |
| MCU | nRF52840 QIAA / NINA-B302-00B |
| Normal update | USB UF2 mass storage |
| Recovery and first program | SWD with OpenOCD or an equivalent probe |
| SoftDevice | S140 v7.3.0 |
| OTA/BLE DFU | Not adopted; `DUALBANK_FW` is disabled |
| Signed update | Not adopted; `SIGNED_FW` is disabled |
| DFU button and LED GPIO | Not defined; their physical connections are unconfirmed |
| UF2 entry | Double-tap physical RESET within 500 ms |
| Temporary USB identity | `0x1209:0x000A`, private-test only; replace before distribution |

`DUALBANK_FW` is not limited to OTA. It reserves a second application bank for
the legacy Nordic DFU transport, but is unnecessary for UF2 and would halve
the available application capacity.

## Adopted Flash map

All bounds are 4 KiB flash-page aligned. The corresponding ZMK bootloader
variant is the source of truth and must use exactly the same bounds.

| Region | Range | Size | Purpose |
| --- | ---: | ---: | --- |
| MBR | `0x00000000-0x00000FFF` | 4 KiB | Nordic MBR |
| S140 v7.3.0 | `0x00001000-0x00026FFF` | 152 KiB | Nordic SoftDevice |
| code_partition | `0x00027000-0x000D3FFF` | 692 KiB | ZMK application |
| storage_partition | `0x000D4000-0x000F3FFF` | 128 KiB | ZMK NVS/settings and bonds |
| bootloader executable | `0x000F4000-0x000FD7FF` | 38 KiB | This bootloader |
| bootloader config | `0x000FD800-0x000FDFFF` | 2 KiB | CF2 configuration |
| MBR parameters | `0x000FE000-0x000FEFFF` | 4 KiB | Nordic MBR parameter page |
| bootloader settings | `0x000FF000-0x000FFFFF` | 4 KiB | Nordic bootloader settings |

There is no secondary application partition. A bootloader-layout ZMK image
must fit within `0x000AD000` bytes (692 KiB). Do not change this map in only
one repository.

## Boot sequence and normal update

```text
RESET -> MBR -> S140 v7.3.0 -> bootloader at 0xF4000
      -> valid ZMK image at 0x27000 -> ZMK
      -> second reset within 500 ms -> USB UF2 + CDC bootloader
```

With no valid application, the bootloader remains in USB DFU. The double-reset
path requires the confirmed physical RESET/NRST path; no matrix GPIO is used
as a DFU trigger.

To update a working device, press RESET twice within 500 ms, wait for the
`TRAVELERS` volume, and copy the bootloader-layout ZMK UF2 onto it. The volume
disconnects when the update completes. UF2 is an unsigned raw-flash update:
only use an image linked at `0x27000` that writes within `code_partition`.

## Build environment

The following Windows/MSYS2 UCRT64 procedure is adapted from the supplied
setup note. Use CMake 3.17 or newer; this repository rejects older versions.

```bash
pacman -Syu
pacman -S \
  git \
  python \
  python-pip \
  mingw-w64-ucrt-x86_64-arm-none-eabi-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-openocd

python -m pip install --user intelhex
git submodule update --init --recursive
```

Confirm the required tools before building:

```bash
python -c "import intelhex; print('intelhex OK')"
arm-none-eabi-gcc --version
cmake --version
ninja --version
openocd --version
```

If required, run these commands from the MSYS2 UCRT64 shell so `/ucrt64/bin`
is on `PATH`. This procedure selects the Ninja generator with `-G Ninja`, so
use `ninja build`, not `make`, after configuration:

```bash
cmake -S . -B build/travelers-board-split -G Ninja \
  -DBOARD=travelers-board-split \
  -DPython_EXECUTABLE=/ucrt64/bin/python.exe
cd build/travelers-board-split
ninja build
```

The CMake build generates these artifacts in `build/travelers-board-split/`:

| File | Purpose |
| --- | --- |
| `bootloader.hex` | Bootloader only, without MBR or S140 |
| `bootloader_mbr.hex` | Bootloader plus MBR; use with a separately programmed S140 |
| `bootloader_mbr.uf2` | Bootloader self-update UF2, not a ZMK application UF2 |

S140 v7.3.0 is read from
`lib/softdevice/s140_nrf52_7.3.0/s140_nrf52_7.3.0_softdevice.hex`.

## Initial SWD program and full recovery

SWDIO, SWCLK, GND and VDD access are confirmed. The following example uses an
ST-LINK; replace its interface configuration if using another debug probe.

```bash
openocd -f interface/stlink.cfg -f target/nrf52.cfg
```

In another terminal, connect to the OpenOCD telnet server:

```bash
telnet localhost 4444
```

For first programming or a full-chip recovery, issue the following in the
OpenOCD terminal. The order is intentional: program S140 first, then the MBR
and bootloader image.

```tcl
reset halt
program /absolute/path/to/lib/softdevice/s140_nrf52_7.3.0/s140_nrf52_7.3.0_softdevice.hex verify
program /absolute/path/to/build/travelers-board-split/bootloader_mbr.hex verify reset
shutdown
```

An erase-all operation removes the bootloader, SoftDevice, ZMK application,
NVS settings, and BLE bonds. Recover by repeating the two programming commands
above, then program a ZMK image built for the bootloader Flash map. Do not
program a direct-boot ZMK image at `0x00000000` after this bootloader is
installed.

## Handoff requirements for the ZMK repository

Create a separate bootloader variant rather than modifying the direct-boot
board. Keep `CONFIG_BOOTLOADER_MCUBOOT=n`; this bootloader is not MCUboot.
Enable a UF2 output for the separate variant, while ZMK USB may remain disabled
because it is not needed by the bootloader.

The DTS must select `code_partition` and reserve all regions below:

```dts
/ {
    chosen {
        zephyr,code-partition = &code_partition;
    };
};

&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        mbr_partition: partition@0 {
            label = "mbr";
            reg = <0x00000000 0x00001000>;
            read-only;
        };
        s140_partition: partition@1000 {
            label = "s140";
            reg = <0x00001000 0x00026000>;
            read-only;
        };
        code_partition: partition@27000 {
            label = "code";
            reg = <0x00027000 0x000AD000>;
        };
        storage_partition: partition@d4000 {
            label = "storage";
            reg = <0x000D4000 0x00020000>;
        };
        bootloader_partition: partition@f4000 {
            label = "bootloader";
            reg = <0x000F4000 0x0000C000>;
            read-only;
        };
    };
};
```

The ZMK application reset vector must therefore be linked at `0x00027000`.
Confirm its final image size is at most `0x000AD000` bytes. SoftDevice and
ZMK/BLE coexistence remains a hardware validation item: test normal boot, BLE,
settings persistence, UF2 update, double reset, and full SWD recovery before
relying on this configuration.

## Unresolved items

- Replace the test-only USB VID/PID before any redistribution.
- Confirm the final hardware behavior of USB enumeration and double-reset.
- Verify ZMK operation with S140 v7.3.0 and the relocated vector table.
- Add a DFU button or LED definition only after its physical GPIO mapping is
  confirmed.
