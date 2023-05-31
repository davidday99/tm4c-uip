# TM4C uIP Port
This repo contains port of uIP to the TM4C MCU. The project structure is based on a template from [tm4c-bare-metal-repo](https://github.com/davidday99/tm4c-bare-metal-template).

## Included Make Recipes
1. `all`: build both an ELF and a flat binary.

2. `clean`: delete build artifacts.

3. `flash`: flash the target with the binary.

## Debugging
The repo includes a script `debug.sh` for debugging the target using `arm-none-eabi-gdb`. 

## Dependencies

[OpenOCD](https://github.com/openocd-org/openocd.git) is necessary for debugging the remote target,
 and [lm4flash](https://github.com/utzig/lm4tools.git) is used to flash it. Both the debug script 
and the makefile are coupled with the `arm-none-eabi` toolchain.

