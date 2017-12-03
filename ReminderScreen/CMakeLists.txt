cmake_minimum_required(VERSION 3.0)

set(RASPBIAN YES)

if (RASPBIAN)
    include(raspbian.cmake)
endif (RASPBIAN)

project(ReminderScreen)

set(project_name ReminderScreen)

set(CMAKE_CXX_STANDARD 14)
set(DEBUG NO)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if (UNIX)
add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
endif(UNIX)

set(SOURCE_FILES
    BusInformationRetriever.cpp
    main.cpp
    ReminderScreen.cpp
    Settings.cpp
)

add_definitions(-DNANA_ENABLE_PNG)
#add_definitions(-DNANA_USE_XFT)

if (DEBUG)
    add_definitions(-DDEBUG)
    add_definitions(-D_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else (DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffunction-sections -fdata-sections")
endif(DEBUG)

include_directories("../Libs/nana/include")
include_directories("../Libs/rapidjson/include")
include_directories("/usr/include/freetype2")

add_subdirectory("../Libs/nana" "${CMAKE_SOURCE_DIR}/../Libs/nana")

add_executable(${project_name} ${SOURCE_FILES})

set(LINKED_LIBRARIES nana Xft fontconfig X11 X11-xcb xcb Xrender Xau Xdmcp freetype expat png dl z
                     boost_system boost_filesystem pthread)

if (DEBUG)
    target_link_libraries(${project_name}
        ${LINKED_LIBRARIES}
    )
else (DEBUG)
    target_link_libraries(${project_name}
        ${LINKED_LIBRARIES}
    )
endif (DEBUG)
