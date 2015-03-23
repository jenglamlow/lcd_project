# =============================================================================
#    FTDI Test Program
# =============================================================================

import serial
from enum import Enum

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


# Color Definition
class Color(Enum):
    red = 0xf800
    green = 0x07e0
    blue = 0x001f
    black = 0x0000
    yellow = 0xffe0
    white = 0xffff

# =============================================================================
#    Helper Function
# =============================================================================


def high_byte(value):
    return (value >> 8) & 0xff


def low_byte(value):
    return (value & 0xff)

# =============================================================================
#    Command Class Definition
# =============================================================================


class Command:

    @property
    def info(self):
        return self._info

    @info.setter
    def info(self, value):
        self._info = value

    @property
    def param(self):
        return self._param

    @param.setter
    def param(self, value):
        self._param = value
        self.construct_packet()

    @property
    def packet(self):
        return self._packet

    @packet.setter
    def packet(self, value):
        pass

    def construct_packet(self):
        param_size = len(self._param) - 1

        # Construct message packet
        packet = []
        packet.append(STX)
        packet.append(self._param[0])
        packet.append(high_byte(param_size))
        packet.append(low_byte(param_size))

        for i in range(len(self._param)):
            if i > 0:
                packet.append(self._param[i])

        packet.append(ETX)

        self._packet = packet

    def __init__(self):
        self._info = ""
        self._param = []
        self._packet = []


class ClearCommand:

    def __init__(self):
        self._command = Command()
        self._command.info = "Clear Display"
        self._command.param = [CMD_CLR]


class BlockCommand:

    def set_param(self, pos0, pos1, color):
        self._pos0 = pos0
        self._pos1 = pos1
        self._color = color

        param = [CMD_BLK]

        # Postion 0
        param.append(high_byte(pos0[0]))
        param.append(low_byte(pos0[0]))
        param.append(high_byte(pos0[1]))
        param.append(low_byte(pos0[1]))

        # Postion 1
        param.append(high_byte(pos1[0]))
        param.append(low_byte(pos1[0]))
        param.append(high_byte(pos1[1]))
        param.append(low_byte(pos1[1]))

        # Color
        param.append(high_byte(color.value))
        param.append(low_byte(color.value))

        self._command.info = "Fill rectangle at " + str(pos0) + " to " \
            + str(pos1) + " with " + color.name
        self._command.param = param

    def __init__(self, pos0, pos1, color):
        self._command = Command()

        BlockCommand.set_param(self, pos0, pos1, color)


class StringCommand:

    def set_param(self, pos, font_size, color, text):
        self._pos = pos
        self._font_size = font_size
        self._color = color
        self._text = text

        param = [CMD_STR]

        # Postion
        param.append(high_byte(pos[0]))
        param.append(low_byte(pos[0]))
        param.append(high_byte(pos[1]))
        param.append(low_byte(pos[1]))

        # Font Size
        param.append(font_size)

        # Color
        param.append(high_byte(color.value))
        param.append(low_byte(color.value))

        # text
        for i in range(len(text)):
            param.append(text[i])

        self._command.info = "Display " + text + " at " + str(pos) + \
            " with font_size " + str(font_size) + " and color " + color.name
        self._command.param = param

    def __init__(self, pos, font_size, color, text):
        self._command = Command()

        StringCommand.set_param(self, pos, font_size, color, text)


# =============================================================================
#    Ftdi Class Definition
# =============================================================================


class Ftdi:

    def __init__(self, device_name, baud):
        self.ser = serial.Serial(device_name, baud)
        if self.ser.isOpen():
            print (self.ser.name, "is opened")
        else:
            print (self.ser.name, "unable to opened")

    def print_info(self, command):
        print ("")
        print ("Sending Command :", command._command.info)
        print ("Packet :", command._command.packet)

    def send(self, command):
        self.print_info(command)
        self.ser.write(bytes(command._command.packet))

    def test_write(self):
        self.ser.write(bytes([2, 3, 0, 0, 3]))


# =============================================================================
#   Class Object Definition
# =============================================================================

# Device FTDI class initialisation
dev = Ftdi('/dev/ttyUSB0', 115200)

# Command Class Initialisation
clear_command = ClearCommand()

block_command = BlockCommand([100, 100], [200, 200], Color.red)

string_command = StringCommand([100, 100], 3, Color.blue, "TEXT")

# =============================================================================
#    Action Function
# =============================================================================


def clear_action():
    dev.send(clear_command)


def block_action():
    dev.send(block_command)


def string_action():
    dev.send(string_command)


def image_action():
    pass


def errHandler():
    print("")
    print ("Invalid Input..")

action_map = {
    'c': clear_action,
    'b': block_action,
    't': string_action,
    'i': image_action,
}

# =============================================================================
#    Main Program
# =============================================================================

print ("")
print ("FTDI testing script")
print ("===================")

while (True):
    print ("")
    print ("c - Clear Display")
    print ("b - Send Block")
    print ("t - Send Text")
    print ("i - Send Image")
    print ("x - Exit")

    c = input("-->")
    if c != 'x':
        action_map.get(c, errHandler)()
    else:
        break

print ("")
print ("Exit")

# =============================================================================
