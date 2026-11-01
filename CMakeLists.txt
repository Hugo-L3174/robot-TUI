cmake_minimum_required (VERSION 3.22)

project(robot-TUI
  LANGUAGES CXX
  VERSION 1.0.0
)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
  GIT_TAG        v7.0.0 # branch name like main is also possible
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)

# easy enum conversion into strings
FetchContent_Declare(magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.9.8
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        8.1.1 # using older fmt version for compatibility with mc_rtc
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)

# Avoid spdlog using the bundled fmt version, it can cause issues when linking with a controller lib that already has another version
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.9.2 # using older spdlog version for compatibility with mc_rtc
  GIT_PROGRESS TRUE
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  c-periphery
  GIT_REPOSITORY https://github.com/vsergeev/c-periphery.git
  GIT_TAG        v2.5.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(c-periphery)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.9.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(yaml-cpp)


# Hardware related things, servos etc
add_library(Hardware
            src/hardware/servos/servo_factory.cpp
            src/hardware/servos/simulated.cpp
            src/hardware/servos/feetech/feetech.cpp
            src/hardware/servos/feetech/scserial.cpp)
target_include_directories(Hardware PUBLIC
                           include/hardware/)
target_link_libraries(Hardware PUBLIC
                      periphery)

# Controller to compute commands and take sensor data
add_library(Controller
            src/controller/controller_factory.cpp
            src/controller/manual.cpp
            src/controller/external.cpp
            src/controller/connection/UDPServer.cpp)

# Add and build mc_rtc controller only if it is found
find_package(mc_rtc QUIET)

if(mc_rtc_FOUND)
  target_sources(Controller PRIVATE
    src/controller/mc_rtc/mc_controller.cpp
    include/controller/mc_rtc/mc_controller.h)

  target_link_libraries(Controller PUBLIC mc_rtc::mc_control)
  # Create variable that can be used in code to check
  target_compile_definitions(Controller PUBLIC ROBOT_TUI_HAS_MC_RTC=1)

  # Some mc_rtc things require c++ 17 instead of 20
  set_source_files_properties(
    src/controller/mc_rtc/mc_controller.cpp
    PROPERTIES COMPILE_OPTIONS "-std=c++17"
  )
endif()

target_include_directories(Controller PUBLIC
                           include/controller/
                           include/hardware/)
target_link_libraries(Controller PUBLIC
                      spdlog::spdlog)

# Central interface connecting all elements
add_library(Interface
            src/interface/interface.cpp)
target_include_directories(Interface PUBLIC
                           include/interface/
                           include/hardware/)
target_link_libraries(Interface PUBLIC
                      Hardware
                      Controller)

# Front end display
add_library(TUI
            src/TUI/terminal.cpp
            src/TUI/panels/connection_panel.cpp
            src/TUI/panels/data_panel.cpp
            src/TUI/panels/controller_panel.cpp)
target_include_directories(TUI PUBLIC
                           include/TUI/)
target_link_libraries(TUI PUBLIC
                      Interface
                      ftxui::screen
                      ftxui::dom
                      ftxui::component
                      fmt::fmt
                      magic_enum::magic_enum
                      spdlog::spdlog
                      yaml-cpp::yaml-cpp)
# Adding path variables for configs folder loading
target_compile_definitions(TUI PRIVATE
  ROBOT_TUI_INSTALL_CONFIG_DIR="${CMAKE_INSTALL_FULL_DATADIR}/robot-TUI/robot_configs"
  ROBOT_TUI_SRC_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/etc/robot_configs"
)

# Main executable
add_executable(robot-TUI
               src/main.cpp)
target_include_directories(robot-TUI PRIVATE
                           include/TUI)
target_link_libraries(robot-TUI PRIVATE
                      TUI)

install(TARGETS robot-TUI
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY etc/robot_configs/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/robot-TUI/robot_configs
        FILES_MATCHING
        PATTERN "*.yaml")
