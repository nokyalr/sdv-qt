# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\sdv-qt_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\sdv-qt_autogen.dir\\ParseCache.txt"
  "sdv-qt_autogen"
  )
endif()
