# ESP32-S3 build and rollback plan

Status: Checkpoint 1 host evidence. No command in this document was used to
flash or access a device. Flashing remains a separately reviewed, human-run
gate on a suitable host, never from WSL2.

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

## Host build environment

The Ubuntu 22.04 WSL2 environment contains:

- ESP-IDF `v6.0.1`, commit
  `8c19b156084a0753687347cca1f5355782893533`
- `xtensa-esp-elf-gcc` 15.2.0
  (`esp-15.2.0_20251204`)
- host GCC 11.4.0
- Python 3.10.12
- `catkin-pkg` 1.1.1, `colcon-core` 0.21.3, `empy` 3.3.4,
  and `lark` 1.3.1 in the ESP-IDF virtual environment
- BNO08x gitlink `fdb99db225fc28f17de0d430a486c182aef927a2`
- pinned micro-ROS revisions listed in `DEPENDENCY_LOCK.md`

The phase snapshot had the BNO08x gitlink but omitted `.gitmodules`. The exact
gitlink was restored for the build, and commit `2f63939d7e71fdfd2e61285f935b9feb7646b40b`
adds the missing submodule URL for reproducible checkouts. The micro-ROS recipe
previously cloned moving branch heads; commit
`8b6d5997f5182b5d78ff7054c45da0ade5c3846f` now detaches every used dependency
at the tested revision and rejects unreviewed EmbeddedRTPS selection.

## Build attempt and result

Tested compiled inputs: commit
`8b6d5997f5182b5d78ff7054c45da0ade5c3846f`. The later `.gitmodules` commit is
metadata-only and does not alter any compiled file.

The generated build directory and micro-ROS output trees were removed, then the
following build was run:

```bash
source /home/karsha-lab/esp/esp-idf-v6.0.1/export.sh
cd /home/karsha-lab/laksa-work-in-phases/firmware/esp32-s3
make -C components/micro_ros_espidf_component -f libmicroros.mk clean
idf.py build
```

The clean build cloned and detached the pinned dependencies, built 62 host
development packages, generated/built 37 embedded micro-ROS packages including
`laksa_interfaces`, and reached ESP-IDF step 1483/1491. It then failed exactly
at the intentional guard in `src/actuator_release_config.h`:

```text
#error "NON-RELEASABLE: review brake action/current, forward/reverse limits,
conversion, and steering endpoints"
```

Exit status was 1. No application image was linked or produced, so binary size,
partition fit, ESP image metadata, and application SHA-256 are unavailable.
Candidate image SHA-256: `NONE`.

Warnings before the guard were from ESP-IDF Kconfig, setuptools/ament, and
micro-ROS compilation (notably GCC `#include_next` pedantic warnings and some
upstream unused variables). One upstream recipe `touch` for an absent optional
`rcl_logging_log4cxx` directory also warned but did not stop package generation.
These warnings are preserved in `/tmp/laksa-esp32-clean-build-8b6d599.log` on
the WSL host; `/tmp` is transient and is not treated as repository evidence.

This build outcome is intentional: unresolved actuator values cannot silently
become a flashable candidate. A successful future compile is still host
evidence only and will not validate braking, steering, movement, or any
physical stop path.

## Reproducible interface checks

`check_ros_interface_layout.py` verifies exact field order before a build.
The source interface SHA-256 values are recorded in `DEPENDENCY_LOCK.md`.
The clean build regenerated the corresponding micro-ROS typesupport rather than
reusing a stale generated tree.

## Human-run app-only flash procedure (draft; do not execute here)

Prerequisites: explicit pre-flash approval, correct physical board/port,
traction battery unplugged, verified full backup above available on the flashing
host, eFuse/security state understood, P3 findings resolved, reviewed actuator
configuration, and a candidate produced from the exact reviewed SHA.

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
