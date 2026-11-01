# robot-TUI

The purpose of this interface is to provide the abstracted structures needed for controlling a robot's hardware, and make them accessible through a TUI based on [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

A lot of high level robot controller frameworks exist, and most commercially available robots provide their own embedded low level interface, but this layer does not always exist on hobby robots.
This interface allows to communicate with the servos or sensors that are connected, display their state in a table, and to connect a higher level controller by giving it the state, and transmitting commands from it.

## Architecture

There are four main elements to this interface:
- The `TUI` layer handles the front-end aspects implementing panels, buttons, fields built with FTXUI.
- The `Interface` layer in the middle manages the lower layers, instanciates the desired hardware classes, and polls their states to keep a buffer accessible for reading.\
It also handles the requests made by the TUI or the connected controller.
- The `Controller` layer is optionally instanciated by the `Interface` and implements the `ControllerInterface` class to process states and output commands from a high level controller.\
Since the logic of this system is to run on the embedded computer of a robot, it is more commonly expected to use the external controller that implements a communication protocol to communicate with a controller on an other machine, but in cases where the controller runs on the same computer connected to the hardware, a regular controller can be used directly.
- The `Hardware` layer implements the `ServoHardware` class for the specific type of motors used, and their communication protocol.\
A simulated dummy class is also implemented for testing.

## Building

#### Dependencies

Requirements:
- CMake > 3.22
- a compiler version that supports C++ 20

Dependencies that are used but pulled and built automatically by CMake FetchContent (you don't need to install them yourself):
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) for the TUI front end
- [magic_enum](https://github.com/Neargye/magic_enum) for easy string parsing of some enums into tui lists
- [fmt](https://github.com/fmtlib/fmt) for easy formatting
- [spdlog](https://github.com/gabime/spdlog) for logging sinks that avoid outputting directly to cout
- [c-periphery](https://github.com/vsergeev/c-periphery) for low level hardware connections
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) for configuration files loading and parsing

#### Instructions

```sh
git clone https://github.com/Hugo-L3174/robot-TUI.git
cd robot-TUI && mkdir build && cd build
cmake ..
cmake --build .
```

Run simply by calling the executable
```sh
./robot-TUI
```
