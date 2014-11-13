#!/bin/bash

# location of OpenOCD Board .cfg files
OPENOCD_BOARD_DIR=/usr/share/openocd/scripts/board

# start xterm with openocd in the background
xterm -e openocd -f $OPENOCD_BOARD_DIR/ek-lm4f120xl.cfg &

# save the PID of the background process
XTERM_PID=$!

# wait a bit to be sure the hardware is ready
sleep 2

if [ $1 == "gdb" ]
then
    arm-none-eabi-gdb --command=init.gdb obj/main.axf
elif [ $1 == "gdbtui" ]
then
    arm-none-eabi-gdb -tui --command=init.gdb obj/main.axf
elif [ $1 == "nemiver" ]
then
    # execute some initialisation commands via gdb
    arm-none-eabi-gdb --batch --command=init.gdb obj/main.axf

    # start the gdb gui
    nemiver --remote=localhost:3333 --gdb-binary="$(which arm-none-eabi-gdb)" obj/main.axf
elif [ $1 == "ddd" ]
then
    ddd --eval-command=init.gdb --debugger arm-none-eabi-gdb obj/main.axf
else
    echo "None of the condition met"
fi

# close xterm when the user has exited nemiver
kill $XTERM_PID
