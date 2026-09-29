# ESP32-S3 build and rollback plan

Status: Checkpoint 0 draft. No command in this document was used to flash a
device. Flashing is a separately reviewed, human-run gate on a suitable host,
not from WSL2.

## Exact rollback asset

The rollback image remains outside Git at:

`/home/karsha-lab/LAKSA_backup_20260926_0422/laksa_car_inventory_20260926_031529_VNPV/esp32/esp32_flash_full.bin`

Recorded and independently recomputed metadata:

- size: 16,777,216 bytes (`0x1000000`)
- SHA-256: `e64712d5cdfc4b6f65d2c7b8ac1f9c2dbdc92c4e3e4b0a12dcdf5763ea991ea9`
- chip: ESP32-S3, QFN56, revision 0.2, 8 MiB PSRAM
- flash: 16 MiB, quad mode, 40 MHz
- read/verify: the recorded `esptool.py` 4.7.0 log verified all
  `0x1000000` bytes from offset `0x0` and recorded the same digest
- factory application: offset `0x10000`, partition size `0x177000`
- factory application image in the backup: 1,341,040 bytes
- recorded ELF SHA-256:
  `fb31999e69da97f8f99459eee75291f84b673eb0df58d0282d5b41805030a17d`

Partition table:

| Name | Offset | Size |
| --- | --- | --- |
| NVS | `0x9000` | `0x6000` |
| PHY init | `0xf000` | `0x1000` |
| factory app | `0x10000` | `0x177000` |

The evidence directory and image were inspected locally. Operator confirmation
that this is the intended restore asset is still pending. The backup does not
capture eFuses; secure-boot/flash-encryption eFuse state was not recorded and
must be checked read-only before any human-run write.

## Build gate

The required toolchain is ESP-IDF 6.0.1 with the project's pinned micro-ROS
dependencies. In Ubuntu 22.04 WSL2, `idf.py` is not on `PATH` and no ESP-IDF
export script was found in the checked standard locations under `/opt/esp` or
`/home/karsha-lab/esp*`. Therefore the ESP-IDF 6.0.1 dependency is currently
**MISSING** and no firmware build has been attempted.

After Checkpoint 0 review and installation of the exact dependency, use a clean
build directory and record:

1. `idf.py --version`, compiler versions, Python version, and micro-ROS revision.
2. Base/source/config/interface SHAs and a clean configure/build command.
3. All warnings, application binary size, partition fit, image metadata, and
   application SHA-256.
4. A reproducible ROS field-order/typesupport check so stale generated types do
   not conceal an ABI mismatch.

A successful compile is host evidence only. It does not validate braking,
steering, movement, or any physical stop path.

## Human-run app-only flash procedure (draft; do not execute here)

Prerequisites: explicit pre-flash approval, correct physical board/port,
traction battery unplugged, verified full backup above available on the flashing
host, eFuse/security state understood, and a candidate produced from the exact
reviewed SHA.

1. Confirm the candidate is an ESP32-S3 application image with
   `esptool.py --chip esp32s3 image_info <candidate.bin>`.
2. Confirm candidate size is no greater than `0x177000`; record `sha256sum`.
3. Re-read and decode the target partition table; require factory app offset
   `0x10000`, size `0x177000`. Abort on any mismatch.
4. Record target chip identity and read-only security/eFuse summary. Abort on a
   different board or unexplained security setting.
5. Write only the factory application:
   `esptool.py --chip esp32s3 --port <ROM_PORT> --baud <APPROVED_BAUD> write_flash --flash_size 16MB 0x10000 <candidate.bin>`
6. Verify the same range:
   `esptool.py --chip esp32s3 --port <ROM_PORT> verify_flash 0x10000 <candidate.bin>`
7. Stop. Do not connect the traction battery or exercise actuators until the
   battery-unplugged identity/interface gate is reviewed.

Do not write bootloader, partition table, NVS, PHY data, or VESC settings during
an app-only trial.

## Human-run full-image emergency restore (draft; do not execute here)

Use only after the operator confirms the exact board and backup asset. A full
restore overwrites the complete external flash, including NVS and calibration
partitions, and is intentionally not part of the normal app-only trial.

1. Recompute and require the exact 16 MiB size and SHA-256 above.
2. Confirm ESP32-S3 identity, 16 MiB flash, and security/eFuse compatibility.
3. Enter ROM download mode using the approved physical procedure.
4. Restore the complete image:
   `esptool.py --chip esp32s3 --port <ROM_PORT> --baud <APPROVED_BAUD> write_flash --flash_size 16MB 0x0 esp32_flash_full.bin`
5. Verify the complete image:
   `esptool.py --chip esp32s3 --port <ROM_PORT> verify_flash 0x0 esp32_flash_full.bin`
6. With traction battery still unplugged, verify installed identity, boot/reset
   reason, raw and typed ROS interfaces, topic/service rates, and startup brake
   state before any further gate.

This procedure cannot restore eFuses. A failed or mismatched verification is a
hard stop. Do not improvise offsets, erase flash first, or substitute a
bootloader-only image.
