# ESP32-S3 host-build dependency lock

Recorded: 2026-09-29 for source
`8b6d5997f5182b5d78ff7054c45da0ade5c3846f`.

This manifest records the exact dependency revisions used by the clean
Checkpoint 1 host build. `pin_dependencies.sh` enforces the micro-ROS revisions
during regeneration; it does not rely on moving Humble branch heads.

## Toolchain and component revisions

| Dependency | Revision/version |
| --- | --- |
| ESP-IDF | `v6.0.1`, `8c19b156084a0753687347cca1f5355782893533` |
| xtensa ESP GCC | `15.2.0`, `esp-15.2.0_20251204` |
| BNO08x component | `fdb99db225fc28f17de0d430a486c182aef927a2` |
| ESP managed-component lock SHA-256 | `50b011d203276217fb24951ec4c611a0a8a02e287ce932121e3747d139cfaf9a` |
| `sdkconfig` SHA-256 | `f2a3fbde00443829a0178996f970fb405dd4276a1de20e89230a6608821b38f2` |
| `sdkconfig.defaults` SHA-256 | `57822665681bf11cc00a760ffc59263fef30b3073f41fab2bdfedffe1bb22655` |
| actuator release guard SHA-256 | `8359ad49ce3cfd2aeffe59c04928ed7c9258343d334aad62f056bf155c85ee4c` |
| micro-ROS recipe SHA-256 | `d85400c736fe51329af79de7a33dbacde78263f77c02bb37f7c5d4ec41323332` |
| micro-ROS pin script SHA-256 | `ed018c3f21a02834f74f6bf1c48dbce5a8dcbcd0e2647019c48acdc9909c1dc9` |

## micro-ROS Humble revisions

| Repository/directory | Exact commit |
| --- | --- |
| `ament_cmake` | `8289d61973f27e66930e0b656e67e12fa1d394cb` |
| `ament_lint` | `a8b535a6d073233fc95b87c3ac4bcb1913a3b046` |
| `ament_package` | `3c8a92b4dda05be83f6e8e879c70f219180e1384` |
| `googletest` | `083afd688a0d14239a831e1734e66a15664389c1` |
| `ament_cmake_ros` | `8ca65783d089348f9138574fb1eff4e20026eea5` |
| `ament_index` | `ae673e815cae9413cfbf4cab6e91a9c1c73c84ef` |
| `Micro-XRCE-DDS-Client` | `83f129a80770a09aac9e823896ecbf6a0eddf0fc` |
| `rmw_microxrcedds` | `3a9856694f6a0273ee6c31e46b4991b424e1a183` |
| `micro-CDR` | `ed4fd513a24a53b93d548d342cb7aa0a18716f04` |
| `rcl` | `600a79c43c028b3659394f12620d67d03915724a` |
| `rclc` | `10eadcc6444a3af3b21e7fb80a626f8c391450e0` |
| `rcutils` | `99316ef43d7eda04cec129b73435f8e4487f1615` |
| `micro_ros_msgs` | `c9062eb3860d16c1bff1423923de3b0956fd4734` |
| `rosidl_typesupport` | `520d9243f17e0cfe53c658b8051ebe2f89ad7c68` |
| `rosidl_typesupport_microxrcedds` | `68b96291bca436243c9f3f9b054e5e117e58f8f7` |
| `rosidl` | `5621b26eff54596a356ad8abd72c06e7650d21f7` |
| `rmw` | `566d17a97e87bc5336ce33fb025ce8e952a3326a` |
| `rcl_interfaces` | `82776fc9068d1e0cd4af11c2e2700195994b2eb1` |
| `rosidl_defaults` | `8266f1c0f062d405595570209cfb0ed3f71684ae` |
| `unique_identifier_msgs` | `27767cefcf8a80da44641dc208c57722c28aa11c` |
| `common_interfaces` | `08434490d3e4abd37ccc5462cdde69b88853b5b8` |
| `example_interfaces` | `ae3df08ac12cea293f53520c7c7dc67cb0cf5355` |
| `test_interface_files` | `a0c8f5e338490ddf8b98238dce35c06810115e8b` |
| `rmw_implementation` | `0fd2967b217afef2d45fbab550b598c70444a0aa` |
| `rcl_logging` | `5bab8cf7a19c466de77aee35fa55778298563c57` |
| `ros2_tracing` | `548634dd2837d65c043436c8186614e924be5c6c` |
| `micro_ros_utilities` | `74fbd3ed2e451e50aea49e45d143905e496c8f10` |

EmbeddedRTPS revisions were not reviewed for this candidate. Selecting that
middleware makes the pin script fail rather than silently building moving or
unreviewed inputs.

## Deployed-interface source hashes

| Definition | SHA-256 |
| --- | --- |
| `DriveCommand.msg` | `5bf531ce8adbe2d33ca9d90717ae710a1a05e2848becf7c591fd19cf5d587fa1` |
| `Pca9685State.msg` | `e87ec6bcd01d91df972ec205c59e3ff44551c805f7036c8da37d6345ba0fa155` |
| `VehicleState.msg` | `20d6766b3ab2093ec2670ecd139898af7261923513e2187fe44d49eb37ea4d79` |
| `VescState.msg` | `19102e93434cb1c294f4d6f14686201e6162fe1ddfe9ea252fc67e0b6e3b7941` |
| `GetVehicleState.srv` | `c062ff79bad0cb4ee86c0cdc816d3595da59377f0187b1d6de96b04f89657b63` |
| `SetDriveCommand.srv` | `8e100f95ec9e07337505cb25ccc736cf0431cd43ba049d9df3bbd095fb14e1ab` |
