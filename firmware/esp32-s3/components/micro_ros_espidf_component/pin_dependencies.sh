#!/bin/sh
set -eu

pin() {
    git -C "src/$1" checkout --detach "$2"
}

case "${1:-}" in
    dev)
        pin ament_cmake 8289d61973f27e66930e0b656e67e12fa1d394cb
        pin ament_lint a8b535a6d073233fc95b87c3ac4bcb1913a3b046
        pin ament_package 3c8a92b4dda05be83f6e8e879c70f219180e1384
        pin googletest 083afd688a0d14239a831e1734e66a15664389c1
        pin ament_cmake_ros 8ca65783d089348f9138574fb1eff4e20026eea5
        pin ament_index ae673e815cae9413cfbf4cab6e91a9c1c73c84ef
        ;;
    runtime)
        if [ -d src/Micro-XRCE-DDS-Client ]; then
            pin Micro-XRCE-DDS-Client 83f129a80770a09aac9e823896ecbf6a0eddf0fc
            pin rmw_microxrcedds 3a9856694f6a0273ee6c31e46b4991b424e1a183
        else
            echo "EmbeddedRTPS revisions are not approved for this candidate." >&2
            exit 1
        fi
        pin micro-CDR ed4fd513a24a53b93d548d342cb7aa0a18716f04
        pin rcl 600a79c43c028b3659394f12620d67d03915724a
        pin rclc 10eadcc6444a3af3b21e7fb80a626f8c391450e0
        pin rcutils 99316ef43d7eda04cec129b73435f8e4487f1615
        pin micro_ros_msgs c9062eb3860d16c1bff1423923de3b0956fd4734
        pin rosidl_typesupport 520d9243f17e0cfe53c658b8051ebe2f89ad7c68
        pin rosidl_typesupport_microxrcedds 68b96291bca436243c9f3f9b054e5e117e58f8f7
        pin rosidl 5621b26eff54596a356ad8abd72c06e7650d21f7
        pin rmw 566d17a97e87bc5336ce33fb025ce8e952a3326a
        pin rcl_interfaces 82776fc9068d1e0cd4af11c2e2700195994b2eb1
        pin rosidl_defaults 8266f1c0f062d405595570209cfb0ed3f71684ae
        pin unique_identifier_msgs 27767cefcf8a80da44641dc208c57722c28aa11c
        pin common_interfaces 08434490d3e4abd37ccc5462cdde69b88853b5b8
        pin example_interfaces ae3df08ac12cea293f53520c7c7dc67cb0cf5355
        pin test_interface_files a0c8f5e338490ddf8b98238dce35c06810115e8b
        pin rmw_implementation 0fd2967b217afef2d45fbab550b598c70444a0aa
        pin rcl_logging 5bab8cf7a19c466de77aee35fa55778298563c57
        pin ros2_tracing 548634dd2837d65c043436c8186614e924be5c6c
        pin micro_ros_utilities 74fbd3ed2e451e50aea49e45d143905e496c8f10
        ;;
    *)
        echo "usage: $0 dev|runtime" >&2
        exit 2
        ;;
esac
