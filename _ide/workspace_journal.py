# 2026-07-04T01:22:29.255026
import vitis

client = vitis.create_client()
client.set_workspace(path="test_vitis3")

platform = client.create_platform_component(name = "platform",hw_design = "$COMPONENT_LOCATION/../../Tarea3/design_1_good_i_think.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0")

comp = client.create_app_component(name="hello_world",platform = "$COMPONENT_LOCATION/../platform/export/platform/platform.xpfm",domain = "standalone_ps7_cortexa9_0",template = "hello_world")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

domain = platform.get_domain(name="zynq_fsbl")

status = domain.regenerate()

status = platform.build()

status = platform.build()

comp.build()

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_lib(lib_name="xilffs", path="$COMPONENT_LOCATION/../../../../FORGE/Xilinx/Vitis/2024.2/data/embeddedsw/lib/sw_services/xilffs_v5_3")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

