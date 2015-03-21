# =============================================================================
#    FTDI Test Program
# =============================================================================

import serial

# =============================================================================
#    Definition
# =============================================================================

# Packet Message Definition
STX = 2
ETX = 3

# Command Definition
CMD_BLK = 0
CMD_IMG = 1
CMD_STR = 2
CMD_CLR = 3

# =============================================================================
#    Command Class Definition
# =============================================================================


class Command:

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def param(self):
        return self._param

    @param.setter
    def param(self, value):
        self._param = value
        self.construct_message()

    @property
    def message(self):
        return self._message

    @message.setter
    def message(self, value):
        pass

    def construct_message(self):
        param_size = len(self._param) - 1
        high_byte = (param_size >> 8) & 0xff
        low_byte = param_size & 0xff

        # Construct message packet
        message = []
        message.append(STX)
        message.append(self._param[0])
        message.append(high_byte)
        message.append(low_byte)

        for i in range(len(self._param)):
            if i > 0:
                message.append(self._param[i])

        message.append(ETX)

        self._message = message

    def __init__(self, name, param):
        self._name = name
        self._param = param
        self._message = []

        self.construct_message()


# =============================================================================
#    Ftdi Class Definition
# =============================================================================


# Command Class Initialisation
clear_command = Command("Clear TFT", [CMD_CLR])
block_command = Command(
    "Draw Block TFT", [CMD_BLK, 0, 0, 0, 100, 0, 200, 0, 200, 2])


class Ftdi:

    def __init__(self, device_name, baud):
        self.ser = serial.Serial(device_name, baud)
        if self.ser.isOpen():
            print (self.ser.name, "is opened")
        else:
            print (self.ser.name, "unable to opened")

    def print_info(self, command):
        print ("")
        print ("Sending Command :", command.name, "-", command.message)

    def clear(self):
        self.print_info(clear_command)
        self.ser.write(bytes(clear_command.message))

    def block(self):
        self.print_info(block_command)
        self.ser.write(bytes(block_command.message))

    def test_write(self):
        self.ser.write(bytes([2, 3, 0, 0, 3]))


# =============================================================================
#    Main Program
# =============================================================================

print ("")
print ("FTDI testing script")
print ("===================")

dev = Ftdi('/dev/ttyUSB0', 115200)
dev.test_write()
#dev.clear()
#dev.block()

print ("")

# =============================================================================
