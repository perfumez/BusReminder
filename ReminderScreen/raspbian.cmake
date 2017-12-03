message("Raspaibn build..")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)
set(CMAKE_ASM_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_FIND_ROOT_PATH /usr/bin)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories(BEFORE SYSTEM "${CMAKE_SOURCE_DIR}/../Libs/raspberry/include")
link_directories("${CMAKE_SOURCE_DIR}/../Libs/raspberry/lib")

#set(ARM_NANA_LIB X11-xcb xcb Xrender Xau Xdmcp freetype expat png dl z)