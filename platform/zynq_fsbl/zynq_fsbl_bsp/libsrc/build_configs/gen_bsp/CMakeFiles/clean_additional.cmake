# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/diskio.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/ff.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/ffconf.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/sleep.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/xilffs.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/xilffs_config.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/xilrsa.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/xiltimer.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/include/xtimer_config.h"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxilffs.a"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxilrsa.a"
  "/home/Yiann/Projects/DASD/test_vitis3/platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxiltimer.a"
  )
endif()
