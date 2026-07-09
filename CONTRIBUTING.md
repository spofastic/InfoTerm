# Contributing

Thank you for your interest in contributing to InfoTerm.

## Principles

- Keep changes small, focused and easy to review.
- Do not hard-code visible UI text. Add or update entries in `include/Language.h` instead.
- Document user-visible changes in `CHANGELOG.md`.
- Never commit Wi-Fi passwords, MQTT passwords, API keys or other secrets.

## Local Testing

1. Open the `InfoTerm` project folder in VS Code with PlatformIO.
2. Install the required libraries through PlatformIO.
3. Adjust the local TFT_eSPI setup if required.
4. Compile the firmware with `platformio run --environment esp32_2432s028r`.
5. Run the host unit tests with `platformio test --environment native`
   (also run automatically in CI). These cover the Arduino-free logic in
   `include/InfoTermLogic.h`. The native environment needs a host C/C++
   compiler on `PATH`:
   - Windows: e.g. MSYS2 (`winget install MSYS2.MSYS2`) or
     `scoop install gcc`, then reopen the terminal so `g++ --version`
     works.
   - Linux/macOS: `gcc`/`clang` from the system package manager or the
     Xcode command line tools.
   Without a compiler, `pio run -e native` / `pio test -e native` fail
   with "g++ not found" - that failure says nothing about the firmware
   build (`esp32_2432s028r` uses its own bundled toolchain).

Both the firmware build and the host tests also run on every pull request via
the PlatformIO GitHub Actions workflow.

## Pull Requests

A pull request should include:

- A short description of the change
- A note whether the firmware was compiled successfully
- A note whether the host unit tests pass (`pio test -e native`)
- Screenshots for web interface or display changes where useful
- Documentation updates when behavior changes
