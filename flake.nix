{
  description = "robot-tui";

  inputs.mc-rtc-nix.url = "github:mc-rtc/nixpkgs";

  outputs =
    inputs:
    inputs.mc-rtc-nix.lib.mkMcRtcModule inputs (
      { lib, ... }:
      {
        flakoboros = {
          packages.robot-tui =
            {
              stdenv,
              lib,
              fetchFromGitHub,
              cmake,
              mc-rtc,
              jrl-cmakemodules,
              fmt,
              ftxui,
              magic-enum,
              spdlog,
              c-periphery,
              yaml-cpp,
              ninja,
            }:
            stdenv.mkDerivation {
              pname = "robot-tui";
              version = "0-unreleased-2026-11-01";

              src = fetchFromGitHub {
                owner = "Hugo-L3174";
                repo = "robot-TUI";
                rev = "8196c3081395d382ffc3c6f26b580253956564f5";
                hash = "sha256-m9Tt/Q3bmbdZ9AqneeDGImygPnDuoYowQujJhoR9Sl8=";
              };

              buildInputs = [ jrl-cmakemodules ];
              nativeBuildInputs = [
                cmake
                ninja
              ];
              propagatedBuildInputs = [
                mc-rtc
                fmt
                ftxui
                magic-enum
                spdlog
                c-periphery
                yaml-cpp
              ];

              cmakeFlags = [
                "-GNinja"
                (lib.cmakeBool "ROBOT_TUI_FORCE_FETCHCONTENT" false)
              ];

              doCheck = true;

              meta = with lib; {
                description = "A low level interface for robots with a TUI to coordinate hardware components and receive commands.";
                homepage = "https://github.com/Hugo-L3174/robot-TUI";
                mainProgram = "robot-TUI";
                license = licenses.asl20;
                platforms = platforms.all;
              };
            };

          overrideAttrs.robot-tui = {
            src = lib.cleanSource ./.;
          };

          # TODO: upstream fix
          packages.c-periphery =
            {
              lib,
              stdenv,
              fetchFromGitHub,
              cmake,
            }:

            stdenv.mkDerivation (_finalAttrs: {
              pname = "c-periphery";
              version = "2.5.2";

              src = fetchFromGitHub {
                owner = "vsergeev";
                repo = "c-periphery";
                tag = "v2.5.2";
                hash = "sha256-SojZS/qN+c78wMv8YUhtebDVst18i0qcTJ1WbMY6dbg=";
              };

              postPatch = ''
                substituteInPlace src/libperiphery.pc.in \
                --replace '=''${prefix}/' '=' \
                --replace '=''${exec_prefix}/' '='
              '';

              nativeBuildInputs = [ cmake ];

              meta = {
                description = "C library for peripheral I/O (GPIO, LED, PWM, SPI, I2C, MMIO, Serial) in Linux";
                homepage = "https://github.com/vsergeev/c-periphery";
                license = lib.licenses.mit;
                maintainers = with lib.maintainers; [ sikmir ];
                platforms = lib.platforms.linux;
              };
            });
        };
      }
    );
}
