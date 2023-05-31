openocd -f /usr/share/openocd/scripts/board/ti_ek-tm4c123gxl.cfg &> .debug.log &

arm-none-eabi-gdb --tui build/uip.elf -ex "target remote :3333" -ex "monitor reset halt"

pkill -P $$

