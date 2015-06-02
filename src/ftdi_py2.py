# =============================================================================
#    FTDI Test Program
# =============================================================================

import serial
import numpy
import time
import datetime
import sys
from PIL import Image

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
CMD_RAW = 4
CMD_SQB = 5


# Color Definition
RED = 0xf800
GREEN = 0x07e0
BLUE = 0x001f
BLACK = 0x0000
YELLOW = 0xffe0
WHITE = 0xffff

# =============================================================================
#    Helper Function
# =============================================================================


def high_word_high_byte(value):
    return (value >> 24) & 0xff


def high_word_low_byte(value):
    return (value >> 16) & 0xff


def high_byte(value):
    return (value >> 8) & 0xff


def low_byte(value):
    return (value & 0xff)


def convert_16_bit_color(r, g, b):
    r = r >> 3
    r = r << 6

    g = g >> 2
    r = r | g
    r = r << 5

    b = b >> 3
    r = r | b

    return r


# =============================================================================
#    Command Class Definition
# =============================================================================


class Command(object):

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
        packet.append(high_word_high_byte(param_size))
        packet.append(high_word_low_byte(param_size))
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


class ClearCommand(object):

    def __init__(self):
        self._command = Command()
        self._command.info = "Clear Display"
        self._command.param = [CMD_CLR]


class BlockCommand(object):

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
        param.append(high_byte(color))
        param.append(low_byte(color))

        self._command.info = "Fill rectangle at " + str(pos0) + " to " \
            + str(pos1) + " with " + str(color)
        self._command.param = param

    def __init__(self, pos0, pos1, color):
        self._command = Command()

        BlockCommand.set_param(self, pos0, pos1, color)


class StringCommand(object):

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
        param.append(high_byte(color))
        param.append(low_byte(color))

        # text
        for i in range(len(text)):
            param.append(ord(text[i]))

        self._command.info = "Display " + text + " at " + str(pos) + \
            " with font_size " + str(font_size) + " and color " + str(color)
        self._command.param = param

    def __init__(self, pos, font_size, color, text):
        self._command = Command()

        StringCommand.set_param(self, pos, font_size, color, text)


class ImageCommand(object):

    def set_param(self, pos, img):
        try:
            self._img = Image.open(img)
        except:
            raise

        self._pos = pos
        self._array = numpy.asarray(self._img)

        # Convert 2D array to 1D
        array_1d = self._array.ravel()

        param = [CMD_IMG]

        # Postion
        param.append(high_byte(pos[0]))
        param.append(low_byte(pos[0]))
        param.append(high_byte(pos[1]))
        param.append(low_byte(pos[1]))

        # Width
        param.append(high_byte(self._img.size[0]))
        param.append(low_byte(self._img.size[0]))
        # Height
        param.append(high_byte(self._img.size[1]))
        param.append(low_byte(self._img.size[1]))

        pixel = int(len(array_1d) / 3)
        for i in range(pixel):
            r = array_1d[(3 * i)]
            g = array_1d[(3 * i) + 1]
            b = array_1d[(3 * i) + 2]

            color = convert_16_bit_color(r, g, b)

            param.append(high_byte(color))
            param.append(low_byte(color))

        self._command.param = param

    def __init__(self, pos, img):
        self._command = Command()

        ImageCommand.set_param(self, pos, img)


class RawCommand(object):

    def set_param(self, cmd, data):
        self._cmd = cmd
        self._data = data

        param = [CMD_RAW]

        # RAW Command
        param.append(cmd)

        # RAW Command Data
        for i in range(len(data)):
            param.append(data[i])

        self._command.param = param

    def __init__(self, cmd, data):
        self._command = Command()

        RawCommand.set_param(self, cmd, data)


class SqbCommand(object):

    def set_param(self, ref, size, color, pos_list):
        self._ref = ref
        self._size = size
        self._color = color
        self._pos_list = pos_list

        param = [CMD_SQB]

        # Reference Position
        param.append(high_byte(ref[0]))
        param.append(low_byte(ref[0]))
        param.append(high_byte(ref[1]))
        param.append(low_byte(ref[1]))

        # Count
        param.append(high_byte(len(pos_list)))
        param.append(low_byte(len(pos_list)))

        # Size
        param.append(high_byte(size[0]))
        param.append(low_byte(size[0]))
        param.append(high_byte(size[1]))
        param.append(low_byte(size[1]))

        # Color
        param.append(high_byte(color))
        param.append(low_byte(color))

        # Position list
        for i in range(len(pos_list)):
            param.append(high_byte(pos_list[i][0]))
            param.append(low_byte(pos_list[i][0]))
            param.append(high_byte(pos_list[i][1]))
            param.append(low_byte(pos_list[i][1]))

        self._command.param = param

    def __init__(self, ref, size, color, pos_list):
        self._command = Command()

        SqbCommand.set_param(self, ref, size, color, pos_list)


# =============================================================================
#    Ftdi Class Definition
# =============================================================================


class Ftdi(object):

    def __init__(self, device_name, baud):
        self.ser = serial.Serial(device_name, baud)
        if self.ser.isOpen():
            print self.ser.name, "is opened"
        else:
            print self.ser.name, "unable to opened"

    def print_info(self, command):
        print ""
        print "Sending Command :", command._command.info
        print "Packet (hex) :"
        if (len(command._command.packet) < 50):
            print ", ".join("{:02x}".format(c)for c in command._command.packet)
        else:
            print ", ".join("{:02x}".format(c)
                            for c in command._command.packet[:50]),
            print "data...", ',',
            print command._command.packet[-1]

    def send(self, command):
        start_time = datetime.datetime.now()
        self.print_info(command)
        self.ser.write(command._command.packet)
        elapsed = datetime.datetime.now() - start_time
        print elapsed

    def test_write(self):
        test_data = [2, 3, 0, 0, 3]
        print "Sending testing command"
        print "Packet :", test_data
        self.ser.write(bytes(test_data))


# =============================================================================
#   Class Object Definition
# =============================================================================

# Device FTDI class initialisation
# dev = Ftdi('/dev/ttyUSB0', 115200)
dev = Ftdi('COM3', 460800)

# Command Class Initialisation
clear_command = ClearCommand()

block_command = BlockCommand([100, 100], [200, 200], RED)

block2_command = BlockCommand([10, 100], [50, 200], BLUE)

string_command = StringCommand([100, 100], 3, YELLOW, "HELLO")

string2_command = StringCommand([10, 200], 3, GREEN, "TESTING")

image_command = ImageCommand([0, 0], "test.bmp")

data_list = []
raw_command = RawCommand(0x01, data_list)

pos_list = [[0, 0], [100, 100], [190, 270]]
sqb_command = SqbCommand([0, 0], [50, 50], WHITE, pos_list)

# =============================================================================
#    Action Function
# =============================================================================


def clear_action():
    dev.send(clear_command)


def block_action():
    dev.send(block_command)
    dev.send(block2_command)


def string_action():
    dev.send(string_command)
    dev.send(string2_command)


def image_action():
    dev.send(image_command)


def raw_action():
    data = [0, 0, 0, 100]
    raw_command.set_param(0x2A, data)
    dev.send(raw_command)
    data = [0, 0, 0, 100]
    raw_command.set_param(0x2B, data)
    dev.send(raw_command)
    data = []
    for i in range(10000):
        data.append(0xF8)
        data.append(0x00)
    raw_command.set_param(0x2C, data)
    dev.send(raw_command)


def sqb_action():
    dev.send(sqb_command)


def test_action():
    clear_action()
    time.sleep(0.5)

    block_action()
    time.sleep(0.5)

    string_action()
    time.sleep(0.5)

    clear_action()
    time.sleep(0.5)

    image_action()


def errHandler():
    print ""
    print "Invalid Input.."

action_map = {
    'c': clear_action,
    'b': block_action,
    't': string_action,
    'i': image_action,
    'r': raw_action,
    's': sqb_action,
    '`': test_action,
}

# =============================================================================
#    Main Program
# =============================================================================

print ""
print "FTDI testing script"
print "==================="

while (True):
    print ""
    print "c - Clear Display"
    print "b - Send Block"
    print "t - Send Text"
    print "i - Send Image"
    print "r - Send Raw"
    print "s - Send Repeated Block"
    print "` - Test Program"
    print "x - Exit"
    print "-->",

    sys.stdout.flush()

    c = raw_input()
    if c != 'x':
        action_map.get(c, errHandler)()
    else:
        break

print ""
print "Exit"

# =============================================================================
