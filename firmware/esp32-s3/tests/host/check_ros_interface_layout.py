#!/usr/bin/env python3
"""Verify the deployed LAKSA ROS field order from source definitions."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
INTERFACES = ROOT / "extra_ros_packages" / "laksa_interfaces"


def fields(path: Path) -> list[str]:
    result = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        value = raw.split("#", 1)[0].strip()
        if value:
            result.append(" ".join(value.split()))
    return result


def check(name: str, actual: list[str], expected: list[str]) -> None:
    if actual != expected:
        print(f"FAIL {name}", file=sys.stderr)
        print(f"expected: {expected}", file=sys.stderr)
        print(f"actual:   {actual}", file=sys.stderr)
        raise SystemExit(1)
    print(f"PASS {name}: {len(expected)} ordered entries")


check(
    "DriveCommand.msg",
    fields(INTERFACES / "msg" / "DriveCommand.msg"),
    [
        "float32 speed_mps",
        "float32 steering_angle_rad",
        "bool brake",
    ],
)

check(
    "VescState.msg",
    fields(INTERFACES / "msg" / "VescState.msg"),
    [
        "builtin_interfaces/Time stamp",
        "bool command_fresh",
        "bool direction_change_pending",
        "bool brake_active",
        "bool telemetry_fresh",
        "uint32 telemetry_sequence",
        "uint32 telemetry_age_ms",
        "int32 requested_erpm",
        "int32 active_erpm",
        "float32 measured_erpm",
        "float32 motor_current_a",
        "float32 input_current_a",
        "float32 duty_cycle",
        "float32 input_voltage_v",
        "float32 amp_hours",
        "float32 amp_hours_charged",
        "float32 watt_hours",
        "float32 watt_hours_charged",
        "float32 temp_mosfet_c",
        "float32 temp_motor_c",
        "float32 pid_position",
        "int32 tachometer",
        "int32 tachometer_abs",
        "uint8 controller_id",
        "uint8 fault_code",
        "float32 motor_angular_velocity_rad_s",
        "float32 wheel_angular_velocity_rad_s",
        "float32 vehicle_linear_velocity_mps",
    ],
)

check(
    "SetDriveCommand.srv",
    fields(INTERFACES / "srv" / "SetDriveCommand.srv"),
    [
        "laksa_interfaces/DriveCommand command",
        "---",
        "bool accepted",
        "uint8 error_code",
    ],
)

check(
    "GetVehicleState.srv",
    fields(INTERFACES / "srv" / "GetVehicleState.srv"),
    [
        "---",
        "laksa_interfaces/VehicleState state",
    ],
)

print("PASS deployed ROS interface source layout (4 definitions)")
