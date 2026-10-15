cmake_minimum_required (VERSION 3.22)

project(servo-interface
  LANGUAGES CXX
  VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt related
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS SerialPort)

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
  GIT_TAG        12.2.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  c-periphery
  GIT_REPOSITORY https://github.com/vsergeev/c-periphery.git
  GIT_TAG        v2.5.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(c-periphery)


# Hardware related things, servos etc
add_library(Hardware
            src/hardware/servos/servo_factory.cpp
            src/hardware/servos/simulated.cpp
            src/hardware/servos/feetech/feetech.cpp
            src/hardware/servos/feetech/scserial.cpp)
target_include_directories(Hardware PUBLIC
                           include/hardware/)
target_link_libraries(Hardware PUBLIC
                      Qt::SerialPort
                      periphery)

# Controller to compute commands and take sensor data
add_library(Controller
            src/controller/controller_factory.cpp
            src/controller/manual.cpp
            src/controller/external.cpp
            src/controller/connection/UDPServer.cpp
            src/controller/mc_rtc/mc_controller.cpp)
            # XXX how to handle conditional include of mc_rtc ?
target_include_directories(Controller PUBLIC
                           include/controller/
                           include/hardware/)
# target_link_libraries(Controller
#                       mc_rtc)

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
                      magic_enum::magic_enum)

# Main executable
add_executable(robot-interface
               src/main.cpp)
target_include_directories(robot-interface PRIVATE
                           include/TUI)
target_link_libraries(robot-interface PRIVATE
                      TUI)
