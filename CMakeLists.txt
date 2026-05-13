cmake_minimum_required(VERSION 3.22)
project(FriendLeaderboard)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

# QPM-generated extern includes will be added by qpm-cmake
include(${CMAKE_CURRENT_LIST_DIR}/cmake/setup.cmake OPTIONAL)

set(COMPILE_ID FriendLeaderboard)

add_library(${COMPILE_ID} SHARED
    src/main.cpp
    src/BeatLeaderAPI.cpp
    src/FriendLeaderboardUI.cpp
    src/SnipeNotification.cpp
    src/ModSettings.cpp
    src/Config.cpp
)

target_include_directories(${COMPILE_ID} PRIVATE
    include
    ${EXTERN_INC_DIR}
)

target_link_libraries(${COMPILE_ID}
    log
    ${EXTERN_LIB_DIR}/libbeatsaber-hook_3_12_1.so
    ${EXTERN_LIB_DIR}/libquestui.so
    ${EXTERN_LIB_DIR}/libbs-utils.so
)

target_compile_options(${COMPILE_ID} PRIVATE
    -frtti
    -fexceptions
    -Wno-invalid-offsetof
    -O2
)

target_compile_definitions(${COMPILE_ID} PRIVATE
    MOD_ID="FriendLeaderboard"
    VERSION="0.1.0"
)
