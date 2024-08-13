# Quansheng UV-K5 driver (c) 2023 Jacek Lipkowski <sq5bpf@lipkowski.org>
#
# based on template.py Copyright 2012 Dan Smith <dsmith@danplanet.com>
#
# This is a preliminary version of a driver for the UV-K5
# It is based on my reverse engineering effort described here:
# https://github.com/sq5bpf/uvk5-reverse-engineering
#
# Warning: this driver is experimental, it may brick your radio,
# eat your lunch and mess up your configuration.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Made From Giorgio (IU0QWJ) for the Fagci R3b0rn firmware
# https://github.com/gpillon
# https://github.com/fagci/uvk5-fagci-reborn
#
# Many Thanks to:
# - IJV
# - Julian Lilov (LZ1JDL)
# - Francesco IK8JHL,
# for their work :)


# Known Bugs (Fixes Needed - Help is Appreciated)
# - Unable to delete rows in the channel list
# - Unable to set Power per channel
# - Unable to set Bandwidth (BW) per channel
# - Custom mode names differ from those on the radio (might require a pull request to CHIRP)

import math

import struct
import logging
import re

# noinspection PyUnresolvedReferences
from chirp import chirp_common, directory, bitwise, memmap, errors, util
# noinspection PyUnresolvedReferences
from chirp.settings import RadioSetting, RadioSettingGroup, \
    RadioSettingValueBoolean, RadioSettingValueList, \
    RadioSettingValueInteger, RadioSettingValueString, \
    RadioSettings, RadioSettingSubGroup

LOG = logging.getLogger(__name__)

# Define the allowed characters
ALLOWED_CHARS = ''.join(chirp_common.CHARSET_ASCII)

# Create a regex pattern to match any character not in the allowed list
ALLOWED_CHARS_PATTERN = f'[^{re.escape(ALLOWED_CHARS)}]'

# Show the obfuscated version of commands. Not needed normally, but
# might be useful for someone who is debugging a similar radio
DEBUG_SHOW_OBFUSCATED_COMMANDS = True

# Show the memory being written/received. Not needed normally, because
# this is the same information as in the packet hexdumps, but
# might be useful for someone debugging some obscure memory issue
DEBUG_SHOW_MEMORY_ACTIONS = True

MEM_BLOCK = 0x80  # largest block of memory that we can reliably write

OFFSET_SIZE = 0  # size of the offset in the memory map

##################
# SRT Fagci Reborn
##################

CHAN_MAX = 1024
READ_WRITE_CHANNELS = CHAN_MAX  # This val should be; CHAN_MAX For debbunging purposes,
                                # this value can be changed to 10 for faster testing (? STILL TRUE??)

SETTINGS_SIZE = 0x001b

PATCH_SIZE = 15832

PATCH_BLOCKS = 1979
# PATCH_SIZE / PATCH_BLOCKS must be an integer!

CH_SIZE = 23

EEPROM_SIZES = [
    8192,  # 000
    8192,  # 001
    8192,  # 010
    16384,  # 011
    32768,  # 100
    65536,  # 101
    131072,  # 110
    262144,  # 111
]

RADIO_LIST = [
    "BK4819",
    "BK1080",
    "SI4732",
    "Preset",
]

BANDWIDTH_LIST = ["25k", "12.5k", "6.25k"]

MODULATION_LIST = ["FM", "AM", "LSB", "USB",
                   "BYP", "RAW", "WFM", "Preset"]

MODULATION_LIST_MAP = ["FM", "AM", "LSB", "USB",
                       "DV", "CW", "WFM", "Auto"]

STEP_NAMES = [
    "0.01kHz",
    "0.1kHz",
    "0.5kHz",
    "1.0kHz",
    "2.5kHz",
    "5.0kHz",
    "6.25kHz",
    "8.33kHz",
    "10.0kHz",
    "12.5kHz",
    "25.0kHz",
    "100.0kHz",
    "125.0kHz",
    "200.0kHz"
]

UPCONVERTER_TYPES = [
    "UPCONVERTER_OFF",
    "UPCONVERTER_50M",
    "UPCONVERTER_125M"
]

OFFSET_DIRECTION = [
    "NONE",
    "+",
    "-"
]

BACKLIGHT_ON_SQUELCH_MODE = [
    "BL_SQL_OFF",
    "BL_SQL_ON",
    "BL_SQL_OPEN"
]

BATTERY_TYPE = [
    "BAT_1600",
    "BAT_2200",
    "BAT_3500"
]

BATTERY_STYLE = [
    "BAT_CLEAN",
    "BAT_PERCENT",
    "BAT_VOLTAGE"
]

TX_OUTPUT_POWER = [
    "TX_POW_LOW",
    "TX_POW_MID",
    "TX_POW_HIGH"
]

RADIO = [
    "RADIO_BK4819",
    "RADIO_BK1080",
    "RADIO_SI4732",
    "RADIO_UNKNOWN"
]

SCAN_TIMEOUT = [
    "0",
    "500ms",
    "1s",
    "2s",
    "5s",
    "10s",
    "30s",
    "1min",
    "2min",
    "5min",
    "NONE"
]

EEPROM_TYPE = [
    "EEPROM_A",
    "EEPROM_B",
    "EEPROM_BL24C64",
    "EEPROM_BL24C128",
    "EEPROM_BL24C256",
    "EEPROM_BL24C512",
    "EEPROM_BL24C1024",
    "EEPROM_M24M02"
]

BL_TIME_VALUES = [0, 5, 10, 20, 60, 120, 255]
BL_TIME_NAMES = ["Off", "5s", "10s", "20s", "1min", "2min", "On"]
BL_TIME_MAP = list(zip(BL_TIME_NAMES, BL_TIME_VALUES))

BL_SQL_MODE_NAMES = ["Off", "On", "Open"]
TX_POWER_NAMES = ["Low", "Mid", "High"]
TX_OFFSET_NAMES = ["Unset", "+", "-"]

TX_CODE_TYPES = RX_CODE_TYPES = ["None", "CTCSS", "DCS", "-DCS"]

EEPROM_TYPE_NAMES = [
    "none 1",  # 000
    "none 2",  # 001
    "BL24C64 (stock)",  # 010
    "BL24C128",  # 011
    "BL24C256",  # 100
    "BL24C512",  # 101
    "BL24C1024",  # 110
    "M24M02 (x1)",  # 111
]

APP_LIST = [
    "None",
    "EEPROM view",
    "Spectrum band",
    "Spectrum analyzer",
    "CH Scan",
    "Channels",
    "Freq catch",
    "1 VFO pro",
    "Frequency input",
    "Run app",
    "Loot",
    "Presets",
    "Reset",
    "Text input",
    "VFO config",
    "Preset config",
    "Settings",
    "1 VFO",
    "2 VFO",
    "ABOUT",
    "Antenna len"
]

BATTERY_TYPE_NAMES = ["1600mAh", "2200mAh", "3500mAh"]
BATTERY_STYLE_NAMES = ["Plain", "Percent", "Voltage"]

SCAN_TIMEOUT_NAMES = [
    "0", "500ms", "1s", "2s", "5s", "10s",
    "30s", "1min", "2min", "5min", "None"
]

BOUND_240_280_NAMES = ["Bound 240", "Bound 280"]
VFOs = ["VFO A", "VFO B"]

SQUELCH_TYPE_LIST = ["RNG", "RG", "RN", "R"]

SQL_CLOSE_NAMES = [f"{i}ms" for i in range(0, 35, 5)]

SQL_OPEN_NAMES = [f"{i}ms" for i in range(0, 15, 5)]

# power
UVK5_POWER_LEVELS = [chirp_common.PowerLevel("Low", watts=1.00),
                     chirp_common.PowerLevel("Med", watts=2.50),
                     chirp_common.PowerLevel("High", watts=5.00)]

BANDS = {
    0: [0.153, 10.000],  # for the SI4732
    1: [10.0, 660.0000],
    2: [840.0000, 1340.0000],
    # 2: [137.0, 173.9990],
    # 3: [174.0, 229.9999],
    # 4: [230.0, 319.9999],
    # 5: [320.0, 399.9999],
    # 6: [400.0, 433.0749],
    # 7: [433.0750, 434.7999],
    # 8: [434.8000, 446.00625],
    # 9: [446.00625, 446.19375],
    # 10: [446.2000, 462.5624],
    # 11: [462.5625, 462.7374],
    # 12: [462.7375, 467.5624],
    # 13: [467.5625, 467.7499],
    # 14: [467.7500, 469.9999],
    # 15: [470.0, 620.0],
    # 16: [840.0, 862.9999],
    # 17: [863.0, 869.9999],
    # 18: [870.0, 889.9999],
    # 19: [890.0, 959.9999],
    # 20: [960.0, 1259.9999],
    # 21: [1260.0, 1299.9999],
    # 22: [1260.0, 1340.0]
}

# BAND_NAMES = [
#     "15-30",
#     "30-64",
#     "64-88",
#     "Bcast FM",
#     "108-118",
#     "Air",
#     "135-144",
#     "2m HAM",
#     "148-174",
#     "174-230",
#     "230-320",
#     "320-400",
#     "400-433",
#     "LPD",
#     "435-446",
#     "PMR",
#     "446-462",
#     "FRS/G462",
#     "462-467",
#     "FRS/G467",
#     "468-470",
#     "470-620",
#     "840-863",
#     "LORA",
#     "870-890",
#     "GSM-900",
#     "960-1260",
#     "23cm HAM",
#     "1.3-1.34"
# ]

TMODES = [None, "Tone", "DTCS", "DTCS"]

CTCSS_TONES = [
    67.0, 69.3, 71.9, 74.4, 77.0, 79.7, 82.5, 85.4,
    88.5, 91.5, 94.8, 97.4, 100.0, 103.5, 107.2, 110.9,
    114.8, 118.8, 123.0, 127.3, 131.8, 136.5, 141.3, 146.2,
    151.4, 156.7, 159.8, 162.2, 165.5, 167.9, 171.3, 173.8,
    177.3, 179.9, 183.5, 186.2, 189.9, 192.8, 196.6, 199.5,
    203.5, 206.5, 210.7, 218.1, 225.7, 229.1, 233.6, 241.8,
    250.3, 254.1,
]

CTCSS_TONES_NAMES = list(map(str, CTCSS_TONES))

DTCS_CODES = [
    23, 25, 26, 31, 32, 36, 43, 47, 51, 53, 54,
    65, 71, 72, 73, 74, 114, 115, 116, 122, 125, 131,
    132, 134, 143, 145, 152, 155, 156, 162, 165, 172, 174,
    205, 212, 223, 225, 226, 243, 244, 245, 246, 251, 252,
    255, 261, 263, 265, 266, 271, 274, 306, 311, 315, 325,
    331, 332, 343, 346, 351, 356, 364, 365, 371, 411, 412,
    413, 423, 431, 432, 445, 446, 452, 454, 455, 462, 464,
    465, 466, 503, 506, 516, 523, 526, 532, 546, 565, 606,
    612, 624, 627, 631, 632, 654, 662, 664, 703, 712, 723,
    731, 732, 734, 743, 754
]

DTCS_CODES_NAMES = list(map(str, DTCS_CODES))

TXPOWER_LIST = ["Low", "Mid", "High"]

MIN_FREQ = 153000
MAX_FREQ = 1399999990

SETTINGS_VF0_PRESET_SIZE = 0x3d1  #  TERRIBLE hardcoding; should get sizeof(MEM_SETTINGS)
MEMORY_START_LOWER_LIMIT = SETTINGS_VF0_PRESET_SIZE

##################
# END Fagci Reborn
##################

READ_SMALL_OFFSET = False

##################
# SRT Memory Map
##################

MEM_SETTINGS = """
struct {
  u8 checkbyte : 5,
     eepromType : 3;
  u8 scrambler:4, 
     squelch : 4;
  u8 vox : 4,
     batsave : 4;
  u8 txTime : 4,
     backlight : 4;
  u8 currentScanlist : 4,
     micGain  : 4;
  u8 chDisplayMode : 2,
     scanmode : 2,
     roger : 2,
     upconverter : 2;
  u8 dtmfdecode : 1,
     repeaterSte : 1,
     ste : 1,
     busyChannelTxLock : 1,
     keylock : 1,
     beep : 1,
     crossBand : 1,
     dw : 1;
  u8 contrast : 4,
     brightness : 4;
  u8 mainApp : 8;
  u8 presetsCount : 8;
  u8 activePreset : 8;
  ul16 batteryStyle : 2,
       batteryType : 2,
       batteryCalibration: 12;
  u8 sqClosedTimeout : 4,
     sqOpenedTimeout : 4;
  u8 backlightOnSquelch : 2,
     reserved2 : 4,
     noListen : 1,
     bound_240_280 : 1;
  u8 scanTimeout : 8;
  u8 activeVFO : 2,
     skipGarbageFrequencies : 1,
     sqlCloseTime : 2,
     sqlOpenTime : 3;
  char nickName[10];
} Settings;
"""

MEM_VF0 = """
struct {
  struct {
    ul32 unused : 1,
      codeType : 4,
      f: 27;
    u8 code;
  } rx;
  struct {
    ul32 unused : 1,
      codeType : 4,
      f: 27;
    u8 code;
  } tx;
  il16  channel;
  u8 radio : 2,
     power : 2,
     modulation : 4;
} VFO[2];
"""

MEM_PRESET = """
struct {
  struct {
    u8 s : 8;
    u8 m : 8;
    u8 e : 8;
  } PowerCalibration;
  ul32 lastUsedFreq_tx_lsb: 5,
       lastUsedFreq_rx: 27;
  ul24 unused: 2,
     lastUsedFreq_tx_msb: 22;
  struct {
    struct {
      ul32 end_lsb: 5,
           start: 27;
      ul24 unused: 2,
         end_msb: 22;
    } FRange;
    char name[10];
    u8 modulation : 4,
       step : 4;
    u8 squelch : 4,
       squelchType : 2,
       bw : 2;
    u8 reserved1 : 3,
       gainIndex : 5;
  } Band;
  u8 memoryBanks : 8;
  u8 unused: 1,
     allowTx : 1,
     radio : 2,
     offsetDir : 2,
     power : 2;
} Preset[29];
"""

MEM_CH = """struct {
  struct {
    ul32 unused : 1,
      codeType : 4,
      f: 27;
    u8 code;
  } rx;
  struct {
    ul32 unused : 1,
      codeType : 4,
      f: 27;
    u8 code;
  } tx;
  char name[10];
  u8 memoryBanks;
  u8 power : 2,
      bw : 2,
      modulation : 4;
  u8  unused: 6,
      radio : 2;
} CH[""" + f"{CHAN_MAX}" + "];\n"


PATCH_BLOCK_SIZE = 4    # patch_data = ul32
# PATCH_BLOCK_SIZE = 2  # patch_data = ul16
# PATCH_BLOCK_SIZE = 1  # patch_data = ul8

MEM_PATCH = """struct {
  ul32 patch_data[""" + f"{int(PATCH_SIZE/PATCH_BLOCKS/PATCH_BLOCK_SIZE)}" + """];
} Patch[""" + f"{PATCH_BLOCKS}" + """];
"""

MEM_FORMAT = MEM_SETTINGS + MEM_VF0 + MEM_PRESET

##################
# END Memory Map
##################

ERROR_TIP = ("\n\nPlease ensure that the radio is **NOT** tuned to a frequency that is receiving a signal,"
          " as this can interrupt the upload / download process.\n\nCheck the cable is connected to the radio and"
          " try again. If everything is ok, just try again, sometimes it requires a few tries")

def sanitize_str(val):
    """)
    Sanitize the nickname to ensure it contains only allowed characters.
    """
    sanitized_name = re.sub(ALLOWED_CHARS_PATTERN, '', str(val)).strip("\x00\xff\x20") + "\x00"
    return sanitized_name


# function to get only first 9 chars of a string and set the 10th char to string terminator
def sanitize_str_10(val):
    """
     Get only the first 9 characters of a string and set the 10th character to string terminator.
     """
    return val[:9] + '\x00'


def min_max_def(value, min_val, max_val, default):
    """returns value if in bounds or default otherwise"""
    if min_val is not None and value < min_val:
        return default
    if max_val is not None and value > max_val:
        return default
    return value


# --------------------------------------------------------------------------------
# nibble to ascii
def hexasc(data):
    res = data
    if res <= 9:
        return chr(res + 48)
    elif data == 0xA:
        return "A"
    elif data == 0xB:
        return "B"
    elif data == 0xC:
        return "C"
    elif data == 0xD:
        return "D"
    elif data == 0xF:
        return "F"
    else:
        return " "


# --------------------------------------------------------------------------------
# nibble to ascii
def ascdec(data):
    if data == "0":
        return 0
    elif data == "1":
        return 1
    elif data == "2":
        return 2
    elif data == "3":
        return 3
    elif data == "4":
        return 4
    elif data == "5":
        return 5
    elif data == "6":
        return 6
    elif data == "7":
        return 7
    elif data == "8":
        return 8
    elif data == "9":
        return 9
    elif data == "A":
        return 10
    elif data == "B":
        return 11
    elif data == "C":
        return 12
    elif data == "D":
        return 13
    elif data == "F":
        return 15
    else:
        return 14


# --------------------------------------------------------------------------------
# the communication is obfuscated using this fine mechanism
def xorarr(data: bytes):
    tbl = [22, 108, 20, 230, 46, 145, 13, 64, 33, 53, 213, 64, 19, 3, 233, 128]
    x = b""
    r = 0
    for byte in data:
        x += bytes([byte ^ tbl[r]])
        r = (r + 1) % len(tbl)
    return x


# --------------------------------------------------------------------------------
# if this crc was used for communication to AND from the radio, then it
# would be a measure to increase reliability.
# but it's only used towards the radio, so it's for further obfuscation
def calculate_crc16_xmodem(data: bytes):
    poly = 0x1021
    crc = 0x0
    for byte in data:
        crc = crc ^ (byte << 8)
        for i in range(8):
            crc = crc << 1
            if (crc & 0x10000):
                crc = (crc ^ poly) & 0xFFFF
    return crc & 0xFFFF


# --------------------------------------------------------------------------------
def _send_command(serport, data: bytes):
    """Send a command to UV-K5 radio"""
    LOG.debug("Sending command (unobfuscated) len=0x%4.4x:\n%s" %
              (len(data), util.hexprint(data)))

    crc = calculate_crc16_xmodem(data)
    data2 = data + struct.pack("<H", crc)

    command = struct.pack(">HBB", 0xabcd, len(data), 0) + \
              xorarr(data2) + \
              struct.pack(">H", 0xdcba)
    if DEBUG_SHOW_OBFUSCATED_COMMANDS:
        LOG.debug("Sending command (obfuscated):\n%s" % util.hexprint(command))
    try:
        result = serport.write(command)
    except Exception:
        raise errors.RadioError("Error writing data to radio{}".format(ERROR_TIP))
    return result


# --------------------------------------------------------------------------------
def _receive_reply(serport):
    header = serport.read(4)
    if len(header) != 4:
        LOG.warning("Header short read: [%s] len=%i" %
                    (util.hexprint(header), len(header)))
        b = serport.read(400)
        LOG.warning("res: \n\n%s\n\n len=%i" % (util.hexprint(b), len(b)))
        raise errors.RadioError("Header short read{}".format(ERROR_TIP))
    if header[0] != 0xAB or header[1] != 0xCD or header[3] != 0x00:
        b = serport.read(400)
        LOG.warning("res: \n\n%s\n\n len=%i" % (util.hexprint(b), len(b)))
        LOG.warning("Bad response header: %s len=%i" %
                    (util.hexprint(header), len(header)))
        raise errors.RadioError("Bad response header{}".format(ERROR_TIP))

    cmd = serport.read(int(header[2]))
    if len(cmd) != int(header[2]):
        LOG.warning("Body short read: [%s] len=%i" %
                    (util.hexprint(cmd), len(cmd)))
        raise errors.RadioError("Command body short read{}".format(ERROR_TIP))

    footer = serport.read(4)

    if len(footer) != 4:
        LOG.warning("Footer short read: [%s] len=%i" %
                    (util.hexprint(footer), len(footer)))
        raise errors.RadioError("Footer short read{}".format(ERROR_TIP))

    if footer[2] != 0xDC or footer[3] != 0xBA:
        LOG.debug(
            "Reply before bad response footer (obfuscated)"
            "len=0x%4.4x:\n%s" % (len(cmd), util.hexprint(cmd)))
        LOG.warning("Bad response footer: %s len=%i" %
                    (util.hexprint(footer), len(footer)))
        raise errors.RadioError("Bad response footer{}".format(ERROR_TIP))

    if DEBUG_SHOW_OBFUSCATED_COMMANDS:
        LOG.debug("Received reply (obfuscated) len=0x%4.4x:\n%s" %
                  (len(cmd), util.hexprint(cmd)))

    cmd2 = xorarr(cmd)

    LOG.debug("Received reply (unobfuscated) len=0x%4.4x:\n%s" %
              (len(cmd2), util.hexprint(cmd2)))

    return cmd2


# --------------------------------------------------------------------------------
def _getstring(data: bytes, begin, maxlen):
    tmplen = min(maxlen + 1, len(data))
    s = [data[i] for i in range(begin, tmplen)]
    for key, val in enumerate(s):
        if val < ord(' ') or val > ord('~'):
            break
    return ''.join(chr(x) for x in s[0:key])


# --------------------------------------------------------------------------------
def _sayhello(serport):
    hellopacket = b"\x14\x05\x04\x00\x6a\x39\x57\x64"

    tries = 5
    while True:
        LOG.debug("Sending hello packet")
        _send_command(serport, hellopacket)
        o = _receive_reply(serport)
        if (o):
            break
        tries -= 1
        if tries == 0:
            LOG.warning("Failed to initialise radio")
            raise errors.RadioError("Failed to initialize radio{}".format(ERROR_TIP))
    firmware = _getstring(o, 4, 16)
    LOG.info("Found firmware: %s" % firmware)
    return firmware


# --------------------------------------------------------------------------------

def _get_offset(serport, offset, length):
    global OFFSET_SIZE
    if OFFSET_SIZE == 0:
        readmem = b"\x1b\x05\x08\x00" + \
                  struct.pack("<HBB", 0, MEM_BLOCK, 0) + \
                  b"\x6a\x39\x57\x64"
        _send_command(serport, readmem)
        header = serport.read(4)
        serport.read(512) # flush the rest of the header
        if len(header) == 4:
            OFFSET_SIZE = 2
            return OFFSET_SIZE
        else:
            readmem = b"\x1b\x05\x0A\x00" + \
                      struct.pack("<IBBBB", 0, MEM_BLOCK, 0, 0, 0) + \
                      b"\x6a\x39\x57\x64"
            _send_command(serport, readmem)
            header = serport.read(4)
            serport.read(512) # flush the rest of the header
            if len(header) == 4:
                OFFSET_SIZE = 4
                return OFFSET_SIZE
            else:
                raise errors.RadioError("Unable to determine offset size{}".format(ERROR_TIP))
    else:
        return OFFSET_SIZE


def _readmem(serport, offset, length):
    current_offset = _get_offset(serport, 0, 0)
    LOG.debug("Offset_len: %d" % current_offset)

    if current_offset == 2:
        LOG.debug("Sending readmem offset=0x%8.4x len=0x%4.4x" % (offset, length))
        readmem = b"\x1b\x05\x08\x00" + \
                  struct.pack("<HBB", offset, length, 0) + \
                  b"\x6a\x39\x57\x64"
    else:
        LOG.debug("Sending readmem offset=0x%4.4x len=0x%4.4x" % (offset, length))
        readmem = b"\x1b\x05\x0A\x00" + \
                  struct.pack("<IBBBB", offset, length, 0, 0, 0) + \
                  b"\x6a\x39\x57\x64"
    _send_command(serport, readmem)
    o = _receive_reply(serport)
    if DEBUG_SHOW_MEMORY_ACTIONS:
        LOG.debug("readmem Received data len=0x%4.4x:\n%s" %
                  (len(o), util.hexprint(o)))
    if OFFSET_SIZE == 2:
        return o[8:]
    else:
        return o[12:]


# --------------------------------------------------------------------------------
def _writemem(serport, data, offset):

    current_offset = _get_offset(serport,0 ,0)

    LOG.debug("Sending writemem offset=0x%4.4x len=0x%4.4x" %
              (offset, len(data)))

    if DEBUG_SHOW_MEMORY_ACTIONS:
        LOG.debug("writemem sent data offset=0x%4.4x len=0x%4.4x:\n%s" %
                  (offset, len(data), util.hexprint(data)))

    dlen = len(data)
    if current_offset == 2:
        writemem = b"\x1d\x05" + \
                   struct.pack("<BBHBB", dlen + 10, 0, offset, dlen, 0) + \
                   b"\x6a\x39\x57\x64" + data
    else:
        writemem = b"\x1d\x05" + \
                   struct.pack("<BBIBBBB", dlen + 10, 0, offset, dlen, 0, 0, 1) + \
                   b"\x6a\x39\x57\x64" + data

    _send_command(serport, writemem)
    o = _receive_reply(serport)

    LOG.debug("writemem Received data: %s len=%i" % (util.hexprint(o), len(o)))

    if (o[0] == 0x1e
            and
            o[4] == (offset & 0xff)
            and
            o[5] == (offset >> 8) & 0xff):
        return True
    else:
        LOG.warning("Bad data from writemem")
        raise errors.RadioError("Bad response to writemem{}".format(ERROR_TIP))


# --------------------------------------------------------------------------------
def _resetradio(serport):
    resetpacket = b"\xdd\x05\x00\x00"
    _send_command(serport, resetpacket)


def set_mem_struct_from_settings(memory_size):
    (ch_memory_start, ch_memory_end, has_patch, _) = get_mem_addrs_and_meta(memory_size)
    LOG.debug("Memory start: %d, Memory end: %d, has_patch : %d" % (ch_memory_start, ch_memory_end, has_patch))

    global MEM_FORMAT
    mem_ch_with_offset = f"\n#seekto {hex(max( SETTINGS_VF0_PRESET_SIZE, ch_memory_start))};\n" + MEM_CH
    MEM_FORMAT = MEM_SETTINGS + MEM_VF0 + MEM_PRESET + mem_ch_with_offset

    if has_patch:
        mem_patch = f"\n#seekto {hex(ch_memory_end)};\n" + MEM_PATCH
        MEM_FORMAT = MEM_FORMAT + mem_patch
        LOG.debug("Memory format with patch: %s" % MEM_FORMAT)


def get_mem_addrs_and_meta(memory_size):
    has_patch = True
    ch_memory_end = memory_size - PATCH_SIZE  # Determine IF PATCH_SIZE is needed
    ch_memory_start = ch_memory_end - (CH_SIZE * CHAN_MAX)
    max_channels = (ch_memory_end - ch_memory_start) // CH_SIZE

    LOG.debug("Memory size: %d, Memory start: %d, Memory end: %d, has_patch : %d" %
              (memory_size, ch_memory_start, ch_memory_end, has_patch))

    if ch_memory_start <= MEMORY_START_LOWER_LIMIT:  # channels.c# 23
        # Not enough memory for all channels, so we need to adjust the memory, unable to fit Patch
        ch_memory_end = memory_size
        ch_memory_start = ch_memory_end - (CH_SIZE * CHAN_MAX)
        has_patch = False

    if ch_memory_start < SETTINGS_VF0_PRESET_SIZE:
        # Not enough memory for all channels, so we need to adjust the memory, unable to fit Patch
        while ch_memory_start < SETTINGS_VF0_PRESET_SIZE:
            ch_memory_start += CH_SIZE
            max_channels -= 1
        # global CHAN_MAX = READ_WRITE_CHANNELS = max_channels
    LOG.debug("Memory size: %d, Memory start: %d, Memory end: %d, has_patch : %d, max_channels = %d" %
              (memory_size, ch_memory_start, ch_memory_end, has_patch, max_channels))

    return ch_memory_start, ch_memory_end, has_patch, max_channels


# ------------------------------READ Eeprom--------------------------------------------------

def map_values(value, left_min, left_max, right_min, right_max):
    # Figure out how 'wide' each range is
    left_span = left_max - left_min
    right_span = right_max - right_min

    # Convert the left range into a 0-1 range (float)
    value_scaled = float(value - left_min) / float(left_span)

    # Convert the 0-1 range into a value in the right range.
    return right_min + (value_scaled * right_span)


def do_download(radio):
    serport = radio.pipe
    serport.timeout = 0.5
    status = chirp_common.Status()
    status.cur = 0
    status.max = SETTINGS_SIZE
    status.msg = "Getting Firmware Version"
    radio.status_fn(status)

    eeprom = b""
    settings = b""
    f = _sayhello(serport)
    if f:
        radio.FIRMWARE_VERSION = f
    else:
        raise errors.RadioError('Unable to determine firmware version')

    sett = 0

    while sett < SETTINGS_SIZE:
        o = _readmem(serport, sett, MEM_BLOCK)
        status.cur = sett
        radio.status_fn(status)

        if o and len(o) == MEM_BLOCK:
            settings += o
            sett += MEM_BLOCK
        else:
            raise errors.RadioError("Memory download incomplete")

    status.msg = "Downloading Settings from radio"
    settings_mmap = memmap.MemoryMapBytes(settings)
    settings_parsed = bitwise.parse(MEM_SETTINGS, settings_mmap)
    eeprom_type = settings_parsed["Settings"]['eepromType']
    memory_size = EEPROM_SIZES[eeprom_type]
    LOG.debug(settings_parsed)

    (ch_memory_start, ch_memory_end, has_patch, max_channels) = get_mem_addrs_and_meta(memory_size)

    LOG.debug("EEPROM type: %d, memory size: %d, memory start: %d, memory end: %d" % (
        eeprom_type, memory_size, ch_memory_start, ch_memory_end))
    set_mem_struct_from_settings(memory_size)

    addr = 0
    status.cur = 0
    status.max = memory_size
    status.msg = f"Downloading Config from radio"

    current_offset = _get_offset(serport, 0, 0)
    if current_offset == 2 and memory_size > 65536:
        raise errors.RadioError("Radio's UART driver has 4 byte offset but memory size is bigger than 512kb\n\nPlease update the firmware (it will be released after 12/08/2024)\n\nMeanwhile just set a smaller memory in the radio (max 512kb)")
    LOG.debug("current_offset: %d, memory_size: %d" % (current_offset, memory_size))

    while addr < SETTINGS_VF0_PRESET_SIZE:
        o = _readmem(serport, addr, MEM_BLOCK)
        status.cur = addr
        radio.status_fn(status)

        if o and len(o) == MEM_BLOCK:
            eeprom += o
            addr += MEM_BLOCK
        else:
            raise errors.RadioError("Config download incomplete")

    status.msg = f"Skipping Empty Bytes"

    while addr < ch_memory_end - (CH_SIZE * READ_WRITE_CHANNELS + MEM_BLOCK):  # WWHHYHYYYY + MEM_BLOCK ?!?!?!?!!?!?!
        o = bytes([0] * MEM_BLOCK)
        status.cur = addr
        radio.status_fn(status)

        if o and len(o) == MEM_BLOCK:
            eeprom += o
            addr += MEM_BLOCK
        else:
            raise errors.RadioError("Empty bytes download incomplete.. WTF??")

    status.msg = f"Downloading Channels from radio"
    while addr < ch_memory_end:
        o = _readmem(serport, addr, MEM_BLOCK)
        status.cur = addr

        status.msg = f"Downloading Channels ({max(0, int(map_values(addr, ch_memory_start, ch_memory_end, 0, max_channels)))} / {max_channels})"
        radio.status_fn(status)
        if o and len(o) == MEM_BLOCK:
            eeprom += o
            addr += MEM_BLOCK
        else:
            raise errors.RadioError("Memory download incomplete")

    status.msg = f"Downloading Patch from radio"
    if has_patch:
        while addr < EEPROM_SIZES[eeprom_type]:
            o = _readmem(serport, addr, MEM_BLOCK)
            status.cur = addr
            status.msg = f"Downloading Patch ({addr - ch_memory_end} / {PATCH_SIZE})"
            radio.status_fn(status)

            if o and len(o) == MEM_BLOCK:
                eeprom += o
                addr += MEM_BLOCK
            else:
                raise errors.RadioError("Patch download incomplete")

    return memmap.MemoryMapBytes(eeprom)


# -------------------------------WRITE EEprom-------------------------------------------------
def do_upload(radio):
    serport = radio.pipe
    serport.timeout = 0.5
    status = chirp_common.Status()
    status.cur = 0
    memory_size = EEPROM_SIZES[radio._memobj.Settings.eepromType]
    (ch_memory_start, ch_memory_end, _, max_channels) = get_mem_addrs_and_meta(memory_size)
    status.max = ch_memory_end

    status.msg = "Uploading VFO Setting to radio"
    radio.status_fn(status)

    current_offset = _get_offset(serport, 0, 0)
    if current_offset == 2 and memory_size > 65536:
        raise errors.RadioError("Radio's UART driver has 4 byte offset but memory size is bigger than 512kb\n\nPlease update the firmware (it will be released after 12/08/2024)\n\nMeanwhile just set a smaller memory in the radio (max 512kb)")
    LOG.debug("current_offset: %d, memory_size: %d" % (current_offset, memory_size))


    f = _sayhello(serport)
    if f:
        radio.FIRMWARE_VERSION = f
    else:
        return False
    # ---------------Write setting
    addr = 0
    status.msg = "Uploading Settings, VFO, Presets to radio"
    while addr < SETTINGS_VF0_PRESET_SIZE:
        o = radio.get_mmap()[addr:addr + MEM_BLOCK]
        _writemem(serport, o, addr)
        status.cur = addr
        radio.status_fn(status)
        if o:
            addr += MEM_BLOCK
        else:
            raise errors.RadioError("Upload Settings, VFO, Presets incomplete")

    # ----------------Empty, just for a nice scrollbar :)
    status.msg = ":) 73s from IU0QWJ :)"
    while addr < ch_memory_start - MEM_BLOCK:  # WWWWWWWWHY - MEM_BLOCK ?!?!?!?!!?!?!
        o = radio.get_mmap()[addr:addr + MEM_BLOCK]
        #  _writemem(serport, o, addr)
        status.cur = addr
        radio.status_fn(status)
        if o:
            addr += MEM_BLOCK
        else:
            raise errors.RadioError("Change your PC, this is not possible; ")

    # ----------------Write Mems
    status.msg = "Uploading Channels (0)"

    while addr < ch_memory_end:
        o = radio.get_mmap()[addr:addr + MEM_BLOCK]
        _writemem(serport, o, addr)
        status.cur = addr
        radio.status_fn(status)
        status.msg = f"Uploading Channels ({max(0, int(map_values(addr, ch_memory_start, ch_memory_end, 0, max_channels)))} / {max_channels})"
        if o:
            addr += MEM_BLOCK
        else:
            raise errors.RadioError("Memory upload incomplete")

    status.msg = "Upload OK"

    _resetradio(serport)

    return True


# --------------------------------------------------------------------------------
def _find_band(hz):
    mhz = hz / 1000000.0

    B = BANDS

    for a in B:
        if mhz >= B[a][0] and mhz <= B[a][1]:
            return a

    return False


def bit_loc(bitnum):
    """
    return the ndx and mask for a bit location
    """
    return (bitnum // 8, 1 << (bitnum & 7))


def store_bit(ch, bank, val):
    """
    store a bit in a bankmem. Store 0 or 1 for False or True
    """
    banknum = bank.index
    ndx, mask = bit_loc(banknum)
    if val:
        ch.memoryBanks |= mask
    else:
        ch.memoryBanks &= ~mask
    return


def retrieve_bit(ch, bank):
    """
    return True or False for a bit in a bankmem
    """
    banknum = bank.index
    ndx, mask = bit_loc(banknum)
    return (ch.memoryBanks & mask) != 0


class FagciBankModel(chirp_common.BankModel):

    def get_num_mappings(self):
        return 8

    def get_mappings(self):
        banks = []
        for i in range(1, 1 + self.get_num_mappings()):
            bank = chirp_common.Bank(self, "%i" % i, "Scan List ")
            bank.index = i - 1
            banks.append(bank)
        return banks

    def add_memory_to_mapping(self, memory, bank):
        chan_max = self._radio.get_features().memory_bounds[1]
        store_bit(self._radio._memobj.CH[chan_max - memory.number], bank, True)

    def remove_memory_from_mapping(self, memory, bank):
        chan_max = self._radio.get_features().memory_bounds[1]
        if not retrieve_bit(self._radio._memobj.CH[chan_max - memory.number], bank):
            raise Exception("Memory %i is not in %s." % (memory.number, bank))

        store_bit(self._radio._memobj.CH[chan_max - memory.number], bank, False)

    # return a list of slots in a bank
    def get_mapping_memories(self, bank):
        memories = []
        chan_max = self._radio.get_features().memory_bounds[1]
        for i in range(*self._radio.get_features().memory_bounds):
            if retrieve_bit(self._radio._memobj.CH[chan_max - i - 1], bank):
                memories.append(self._radio.get_memory(chan_max - i - 1))
        return memories

    # return a list of banks a slot is a member of
    def get_memory_mappings(self, memory):
        chan_max = self._radio.get_features().memory_bounds[1]
        banks = []
        LOG.debug(self._radio._memobj.CH[chan_max - memory.number].memoryBanks)
        for bank in self.get_mappings():
            if retrieve_bit(self._radio._memobj.CH[chan_max - memory.number], bank):
                banks.append(bank)
        return banks


def set_tx_code(_mem, curr_vfo, i, v):
    # tx_code
    key_name = f"vfo{i}_tx_code"
    tx_code = int(curr_vfo.tx.code)
    tx_codeType = int(_mem.VFO[i].tx.codeType)
    if (key_name in v):
        del v[key_name]
    if TX_CODE_TYPES[tx_codeType] == "CTCSS":
        if tx_code >= len(CTCSS_TONES):
            tx_code = 0
        v.append(RadioSetting(key_name, "TX Code",
                              RadioSettingValueList(CTCSS_TONES_NAMES, CTCSS_TONES_NAMES[tx_code]))
                 )
    elif TX_CODE_TYPES[tx_codeType] == "DCS" or TX_CODE_TYPES[tx_codeType] == "-DCS":
        if tx_code >= len(DTCS_CODES):
            tx_code = 0
        v.append(RadioSetting(key_name, "TX Code",
                              RadioSettingValueList(DTCS_CODES_NAMES, DTCS_CODES_NAMES[tx_code]))
                 )
    else:
        rs = RadioSettingValueString(0, 0, "")
        rs.set_mutable(False)
        v.append(RadioSetting(key_name, "TX Code", rs))


def set_rx_code(_mem, curr_vfo, i, v):
    # rx_code
    key_name = f"vfo{i}_rx_code"
    rx_code = int(curr_vfo.rx.code)
    rx_code_type = int(_mem.VFO[i].rx.codeType)
    if RX_CODE_TYPES[rx_code_type] == "CTCSS":
        if rx_code >= len(CTCSS_TONES):
            rx_code = 0
        v.append(RadioSetting(key_name, "RX Code",
                              RadioSettingValueList(CTCSS_TONES_NAMES, CTCSS_TONES_NAMES[rx_code]))
                 )
    elif RX_CODE_TYPES[rx_code_type] == "DCS" or RX_CODE_TYPES[rx_code_type] == "-DCS":
        if rx_code >= len(DTCS_CODES):
            rx_code = 0
        v.append(RadioSetting(key_name, "RX Code",
                              RadioSettingValueList(DTCS_CODES_NAMES, DTCS_CODES_NAMES[rx_code]))
                 )
    else:
        rs = RadioSettingValueString(0, 0, "")
        rs.set_mutable(False)
        v.append(RadioSetting(key_name, "RX Code", rs))


@directory.register
class UVK5Radio(chirp_common.CloneModeRadio):
    """Quansheng UV-K5"""
    VENDOR = "Quansheng"
    MODEL = "UV-K5"
    VARIANT = "FAGCI-REBORN"
    BAUD_RATE = 38400
    NEEDS_COMPAT_SERIAL = False
    FIRMWARE_VERSION = "300"
    _expanded_limits = True

    def get_bank_model(self):
        return FagciBankModel(self)

    # --------------------------------------------------------------------------------
    def get_prompts(x=None):
        rp = chirp_common.RadioPrompts()
        rp.experimental = _(
            'This is an experimental driver for the Quansheng UV-K5. '
            'It may harm your radio, or worse. Use at your own risk.\n\n'
            'Before attempting to do any changes please download '
            'the memory image from the radio with chirp '
            'and keep it. This can be later used to recover the '
            'original settings. \n\n'
            'some details are not yet implemented')
        rp.pre_download = _(
            "1. Turn radio on.\n"
            "2. Connect cable to mic/spkr connector.\n"
            "3. Make sure connector is firmly connected.\n"
            "4. Click OK to download image from device.\n\n"
            "It will may not work if you turn on the radio "
            "with the cable already attached\n")
        rp.pre_upload = _(
            "1. Turn radio on.\n"
            "2. Connect cable to mic/spkr connector.\n"
            "3. Make sure connector is firmly connected.\n"
            "4. Click OK to upload the image to device.\n\n"
            "It will may not work if you turn on the radio "
            "with the cable already attached")
        return rp

    # --------------------------------------------------------------------------------
    # Return information about this radio's features, including
    # how many memories it has, what bands it supports, etc
    def get_features(self):
        rf = chirp_common.RadioFeatures()
        rf.has_bank = True
        rf.has_bank_index = True
        rf.has_rx_dtcs = True
        rf.has_ctone = True
        rf.has_settings = True
        rf.has_comment = True
        rf.has_nostep_tuning = True
        rf.has_tuning_step = False
        rf.valid_dtcs_codes = DTCS_CODES
        rf.valid_name_length = 10
        rf.valid_power_levels = UVK5_POWER_LEVELS
        #rf.valid_duplexes = ["", "off", "-", "+"]
        #rf.valid_tuning_steps = STEPS
        rf.valid_tmodes = ["", "Tone", "TSQL", "DTCS", "Cross"]
        rf.valid_cross_modes = ["Tone->Tone", "Tone->DTCS", "DTCS->Tone", "->Tone", "->DTCS", "DTCS->", "DTCS->DTCS"]
        rf.valid_characters = chirp_common.CHARSET_ASCII
        rf.valid_modes = MODULATION_LIST_MAP
        rf.valid_tuning_steps = []

        # rf.valid_skips = ["", "S"]
        rf.valid_skips = []
        rf._expanded_limits = True
        rf.memory_bounds = (1, self.max_channels)

        for a in BANDS:
            rf.valid_bands.append(
                (
                    int(BANDS[a][0] * 1000000),
                    int(BANDS[a][1] * 1000000)
                )
            )
        return rf

    # --------------------------------------------------------------------------------
    # Do a download of the radio from the serial port
    def sync_in(self):
        self._mmap = do_download(self)
        self.process_mmap()

    # --------------------------------------------------------------------------------
    # Do an upload of the radio to the serial port
    def sync_out(self):
        do_upload(self)

    # --------------------------------------------------------------------------------
    # Convert the raw byte array into a memory object structure
    def process_mmap(self):

        settings_parsed = bitwise.parse(MEM_SETTINGS, self._mmap)
        eeprom_type = settings_parsed["Settings"]['eepromType']
        memory_size = EEPROM_SIZES[eeprom_type]
        set_mem_struct_from_settings(memory_size)
        self._memobj = bitwise.parse(MEM_FORMAT, self._mmap)
        (_, _, _, max_channels) = get_mem_addrs_and_meta(memory_size)
        self.max_channels = max_channels

        # # This code works, But is what we want?
        # real_bands = [(10.0000, 660.0000), (840.0000, 1340.0000)]
        # for i in range(0, len(self._memobj.Preset)):
        #     curr_pr = self._memobj.Preset[i]
        #     start_freq = curr_pr.Band.FRange.start * 10
        #     end_freq = curr_pr.Band.FRange.end_msb << 5 | curr_pr.Band.FRange.end_lsb * 10
        #     real_bands = (start_freq/ 1000 / 1000, end_freq / 1000 / 1000)
        # chirp_common.RadioFeatures().valid_bands = real_bands

    # --------------------------------------------------------------------------------
    # Return a raw representation of the memory object, which
    # is very helpful for development
    def get_raw_memory(self, number):
        return repr(self._memobj.CH[number - 1])

    # --------------------------------------------------------------------------------
    def validate_memory(self, mem):
        msgs = super().validate_memory(mem)

        if mem.name == "" or mem.name == "<new>":
            mem.name = f"{(mem.freq / 1000 / 1000):.4f}"
            self._memobj.CH[self.max_channels - mem.number].memoryBanks = 0x00

        if mem.duplex not in ['', "-", "+"]:
            msgs.append(chirp_common.ValidationError("Invalid duplex setting: %s" % mem.duplex))

        # find tx frequency
        if mem.duplex == '-':
            txfreq = mem.freq - mem.offset
        elif mem.duplex == '+':
            txfreq = mem.freq + mem.offset
        else:
            txfreq = mem.freq

        # find band
        band = _find_band(txfreq)
        if band is False:
            msg = "Transmit frequency %.4f MHz is not supported by this radio" % (txfreq / 1000000.0)
            msgs.append(chirp_common.ValidationError(msg))

        band = _find_band(mem.freq)
        if band is False:
            msg = "The frequency %.4f MHz is not supported by this radio" % (mem.freq / 1000000.0)
            msgs.append(chirp_common.ValidationError(msg))

        return msgs

    # --------------------------------------------------------------------------------
    def _set_tone(self, mem, _mem):
        ((txmode, txtone, txpol),
         (rxmode, rxtone, rxpol)) = chirp_common.split_tone_encode(mem)

        if txmode == "Tone":
            txmoval = TX_CODE_TYPES.index("CTCSS")
            txtoval = CTCSS_TONES.index(txtone)
        elif txmode == "DTCS":
            txmoval = txpol == "R" and TX_CODE_TYPES.index("-DCS") or TX_CODE_TYPES.index("DCS")
            txtoval = DTCS_CODES.index(txtone)
        else:
            txmoval = TX_CODE_TYPES.index("None")
            txtoval = 0

        if rxmode == "Tone":
            rxmoval = RX_CODE_TYPES.index("CTCSS")
            rxtoval = CTCSS_TONES.index(rxtone)
        elif rxmode == "DTCS":
            rxmoval = rxpol == "R" and RX_CODE_TYPES.index("-DCS") or RX_CODE_TYPES.index("DCS")
            rxtoval = DTCS_CODES.index(rxtone)
        else:
            rxmoval = RX_CODE_TYPES.index("None")
            rxtoval = 0

        _mem.rx.codeType = rxmoval
        _mem.tx.codeType = txmoval
        _mem.rx.code = rxtoval
        _mem.tx.code = txtoval

    # --------------------------------------------------------------------------------
    def _get_tone(self, mem, _mem):
        rx_pol = None
        tx_pol = None

        rxtype = _mem.rx.codeType
        txtype = _mem.tx.codeType

        if rxtype >= len(TMODES):
            rxtype = 0
        rx_tmode = TMODES[rxtype]

        if txtype >= len(TMODES):
            txtype = 0
        tx_tmode = TMODES[txtype]

        rx_tone = tx_tone = "None"

        if tx_tmode == "Tone":
            if _mem.tx.code < len(CTCSS_TONES):
                tx_tone = CTCSS_TONES[_mem.tx.code]
            else:
                tx_tone = 0
        elif tx_tmode == "DTCS":
            if _mem.tx.code < len(DTCS_CODES):
                tx_tone = DTCS_CODES[_mem.tx.code]
                tx_pol = TX_CODE_TYPES[txtype] == "DCS" and "N" or "R"
            else:
                tx_tone = 0

        if rx_tmode == "Tone":
            if _mem.rx.code < len(CTCSS_TONES):
                rx_tone = CTCSS_TONES[_mem.rx.code]
            else:
                rx_tone = 0
        elif rx_tmode == "DTCS":
            if _mem.rx.code < len(DTCS_CODES):
                rx_tone = DTCS_CODES[_mem.rx.code]
                rx_pol = RX_CODE_TYPES[rxtype] == "DCS" and "N" or "R"
            else:
                rx_tone = 0

        tx_tmode = TMODES[txtype]
        rx_tmode = TMODES[rxtype]

        chirp_common.split_tone_decode(mem, (tx_tmode, tx_tone, tx_pol), (rx_tmode, rx_tone, rx_pol))

    # --------------------------------------------------------------------------------
    # Extract a high-level memory object from the low-level memory map
    # This is called to populate a memory in the UI
    def get_memory(self, number2):

        _mem = self._memobj

        mem = chirp_common.Memory()

        number = self.max_channels - (number2)

        mem.number = self.max_channels - number

        _mem = self._memobj.CH[number]

        # if number > (self.max_channels - 1):
        #     mem.immutable = ["name", "scanlists"]
        # else:
        for char in _mem.name:
            if str(char) == "\xFF" or str(char) == "\x00":
                break
            mem.name += str(char)

        tag = mem.name.strip()
        mem.name = tag

        is_empty = False
        # We'll consider any blank (i.e. 0 MHz frequency) to be empty
        if (_mem.rx.f == 0xffffffff) or (_mem.rx.f == 0) or (_mem.rx.f == 0x7FFFFFF) or mem.name == "":
            is_empty = True

        if is_empty:
            mem.empty = True
            # set some sane defaults:
            mem.power = UVK5_POWER_LEVELS[0]
            mem.extra = RadioSettingGroup("Extra", "extra")

            rs = RadioSetting("radio", "Radio", RadioSettingValueList(RADIO_LIST, RADIO_LIST[0]))
            mem.extra.append(rs)

            rs = RadioSetting("bandwidth", "Bandwidth", RadioSettingValueList(BANDWIDTH_LIST, BANDWIDTH_LIST[1]))
            mem.extra.append(rs)

            mem.duplex = ""
            mem.mode = MODULATION_LIST_MAP[0]
            mem.offset = 0

            return mem

        # Convert your low-level frequency to Hertz
        mem.freq = int(_mem.rx.f) * 10
        freq_tx = int(_mem.tx.f) * 10
        offset = 0 if freq_tx == 0 or (_mem.tx.f == 0x7FFFFFF) else int(freq_tx - mem.freq)

        # LOG.debug(f"name: {mem.name}, mem.freq: {mem.freq}, freq_tx: {freq_tx}, mem.offset: {mem.offset}")

        if offset == 0:
            mem.duplex = ''
        elif offset < 0:
            mem.duplex = '-'
        else:
            mem.duplex = '+'

        mem.offset = abs(offset)
        # tone data
        self._get_tone(mem, _mem)

        # mode
        if _mem.modulation >= len(MODULATION_LIST_MAP):
            mem.mode = MODULATION_LIST_MAP[0]
        else:
            mem.mode = MODULATION_LIST_MAP[_mem.modulation]

        # power
        if _mem.power >= len(UVK5_POWER_LEVELS):
            mem.power = UVK5_POWER_LEVELS[0]
        else:
            mem.power = UVK5_POWER_LEVELS[_mem.power]

        # We'll consider any blank (i.e. 0 MHz frequency) to be empty
        if (_mem.rx.f == 0xffffffff) or (_mem.rx.f == 0) or (_mem.rx.f == 0x7fffffff) or mem.name == "":
            mem.empty = True
        else:
            mem.empty = False

        mem.extra = RadioSettingGroup("Extra", "extra")

        # bandwidth
        bwidth = _mem.bw
        if bwidth >= len(BANDWIDTH_LIST):
            bwidth = 0
        rs = RadioSetting("bandwidth", "Bandwidth", RadioSettingValueList(BANDWIDTH_LIST, BANDWIDTH_LIST[bwidth]))
        mem.extra.append(rs)

        radio = _mem.radio
        rs = RadioSetting("radio", "Radio", RadioSettingValueList(RADIO_LIST, RADIO_LIST[radio]))
        mem.extra.append(rs)

        return mem

    # --------------------------------------------------------------------------------
    # Store details about a high-level memory to the memory map
    # This is called when a user edits a memory in the UI
    def set_memory(self, mem):

        number = self.max_channels - mem.number

        if number > self.max_channels:
            return mem

        # Get a low-level memory object mapped to the image
        _mem = self._memobj.CH[number]

        old_rx_freq = _mem.rx.f
        # frequency/offset
        _mem.rx.f = int(mem.freq / 10)
        rx_delta = old_rx_freq - _mem.rx.f
        # _mem.tx.f = mem.freq / 10
        # _mem.offset = mem.offset / 10

        LOG.debug(
            f"mem.freq: {mem.freq}, _mem.rx.f: {int(_mem.rx.f)} mem.duplex: {mem.duplex}, "
            f"_mem.tx.f: {int(_mem.tx.f)}, mem.offset: {mem.offset}"
        )

        if mem.duplex == '-':
            _mem.tx.f = int(_mem.rx.f - mem.offset / 10)
            # _mem.tx.f += rx_delta  # FIXME: this is a hack to keep the offset the same, but it's not working
        elif mem.duplex == '+':
            _mem.tx.f = int(_mem.rx.f + mem.offset / 10)
            # _mem.tx.f += rx_delta  # FIXME: this is a hack to keep the offset the same, but it's not working
        elif mem.duplex == '':
            _mem.tx.f = 0

        _mem.tx.f += rx_delta

        LOG.debug(
            f"mem.freq: {mem.freq}, _mem.rx.f: {int(_mem.rx.f)} mem.duplex: {mem.duplex}, "
            f"_mem.tx.f: {int(_mem.tx.f)},  mem.offset: {mem.offset}"
        )

        # name
        tag = mem.name.ljust(10)
        if mem.freq == 0:
            _mem.name = b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            return mem
        _mem.name = sanitize_str_10(tag)

        # tone data
        self._set_tone(mem, _mem)

        # tx power
        if str(mem.power) == str(UVK5_POWER_LEVELS[2]):
            _mem.power = TXPOWER_LIST.index("High")
        elif str(mem.power) == str(UVK5_POWER_LEVELS[1]):
            _mem.power = TXPOWER_LIST.index("Mid")
        else:
            _mem.power = TXPOWER_LIST.index("Low")

        _mem.modulation = MODULATION_LIST_MAP.index(mem.mode)

        for setting in mem.extra:
            sname = setting.get_name()
            svalue = setting.value.get_value()

            # LOG.debug(f"---------------------\n-------------------\n------------------\nSetting: {sname} Value: {svalue}\n-----------------\n---------------------\n-----------------")

            if sname == "bandwidth":
                _mem.bw = BANDWIDTH_LIST.index(svalue)

            if sname == "radio":
                _mem.radio = RADIO_LIST.index(svalue)
        LOG.debug("--------------end of set_memory-----------------")
        return mem

    # --------------------------------------------------------------------------------
    def get_settings(self):
        _mem = self._memobj

        basic = RadioSettingGroup("basic", "Basic Settings")
        display = RadioSettingGroup("Display", "Display")
        sql = RadioSettingGroup("sql", "SQL")
        patch = RadioSettingGroup("patch", "Patch")

        # --------------------------------------------------------------------------------
        # helper function
        def append_label(radio_setting, label, descr=""):
            if not hasattr(append_label, 'idx'):
                append_label.idx = 0

            append_label_val = RadioSettingValueString(len(descr), len(descr), descr)
            append_label_val.set_mutable(False)
            append_label_rs = RadioSetting("label" + str(append_label.idx), label, append_label_val)
            append_label.idx += 1
            radio_setting.append(append_label_rs)

        # readonly info
        # Firmware
        #firmware = self.metadata.get('uvk5_firmware', 'UNKNOWN')
        firmware = self.FIRMWARE_VERSION

        val = RadioSettingValueString(0, 128, firmware)
        val.set_mutable(False)
        rs = RadioSetting("fw_ver", "Firmware Version", val)
        basic.append(rs)

        # append_label(basic, f"Last Used Freq Offset 1: {_mem.lastUsedFreq_offset_2}")
        # append_label(basic, f"Last Used Freq Offset 2: {_mem.lastUsedFreq_offset_2}")

        vfo = []
        for i in range(len(_mem.VFO)):
            curr_vfo = _mem.VFO[i]
            v = RadioSettingGroup(f"vfo{i}", f"VFO {i + 1}")

            #tx_f
            tx = int(curr_vfo.tx.f)
            v.append(
                RadioSetting(f"vfo{i}_tx_f", "TX Frequency", RadioSettingValueInteger(MIN_FREQ, MAX_FREQ, tx * 10)))

            #tx_codeType
            tx_codeType = int(curr_vfo.tx.codeType)
            if tx_codeType >= len(TX_CODE_TYPES):
                tx_codeType = 0
            v.append(RadioSetting(f"vfo{i}_tx_codeType", "TX Code Type",
                                  RadioSettingValueList(TX_CODE_TYPES, TX_CODE_TYPES[tx_codeType])))

            set_tx_code(_mem, curr_vfo, i, v)

            #rx_f
            rx = int(curr_vfo.rx.f)
            v.append(
                RadioSetting(f"vfo{i}_rx_f", "RX Frequency", RadioSettingValueInteger(MIN_FREQ, MAX_FREQ, rx * 10)))

            #rx_codeType
            rx_codeType = int(curr_vfo.rx.codeType)
            if rx_codeType >= len(RX_CODE_TYPES):
                rx_codeType = 0
            v.append(RadioSetting(f"vfo{i}_rx_codeType", "RX Code Type",
                                  RadioSettingValueList(RX_CODE_TYPES, RX_CODE_TYPES[rx_codeType])))

            set_rx_code(_mem, curr_vfo, i, v)

            #channel
            channel = curr_vfo.channel
            if channel >= len(_mem.CH) or channel < -1:
                channel = 0
            v.append(RadioSetting(f"vfo{i}_channel", "VFO Channel", RadioSettingValueInteger(0, self.max_channels, channel + 1)))

            #modulation
            modulation = int(curr_vfo.modulation)
            if modulation >= len(MODULATION_LIST_MAP):
                modulation = 0
            v.append(RadioSetting(f"vfo{i}_modulation", "VFO Modulation",
                                  RadioSettingValueList(MODULATION_LIST_MAP, MODULATION_LIST_MAP[modulation])))

            #power
            power = curr_vfo.power
            if power >= len(UVK5_POWER_LEVELS):
                power = 0
            v.append(RadioSetting(f"vfo{i}_power", "VFO Power",
                                  RadioSettingValueList(TXPOWER_LIST, TXPOWER_LIST[power])))

            #radio
            radio = curr_vfo.radio
            if radio >= len(RADIO_LIST):
                radio = 0
            v.append(RadioSetting(f"vfo{i}_radio", "Radio", RadioSettingValueList(RADIO_LIST, RADIO_LIST[radio])))

            vfo.append(v)

        presets = []
        for i in range(len(_mem.Preset)):

            curr_pr = _mem.Preset[i]
            band = curr_pr.Band
            name = sanitize_str(band.name)
            n = str(name).strip("\x20\x00\xff")
            pr = RadioSettingGroup(f"preset{i}", f"Preset {i + 1} ({n})")

            # LOG.debug(f"Processing preset {i}")
            # LOG.debug(f"Current preset: {curr_pr}")

            # PowerCalibration.s
            pr.append(RadioSetting(f"preset{i}_PowerCalibration_s", "Power Calibration S",
                                   RadioSettingValueInteger(0, 255, curr_pr.PowerCalibration.s)))

            # PowerCalibration.m
            pr.append(RadioSetting(f"preset{i}_PowerCalibration_m", "Power Calibration M",
                                   RadioSettingValueInteger(0, 255, curr_pr.PowerCalibration.m)))

            # PowerCalibration.e
            pr.append(RadioSetting(f"preset{i}_PowerCalibration_e", "Power Calibration E",
                                   RadioSettingValueInteger(0, 255, curr_pr.PowerCalibration.e)))

            # start
            start_freq = curr_pr.Band.FRange.start
            start_freq_rs = RadioSettingValueInteger(0, MAX_FREQ, start_freq * 10)
            pr.append(RadioSetting(f"preset{i}_band_start", "Start Frequency", start_freq_rs))

            # end
            end_freq = curr_pr.Band.FRange.end_msb << 5 | curr_pr.Band.FRange.end_lsb
            end_freq_rs = RadioSettingValueInteger(0, MAX_FREQ, end_freq * 10)
            pr.append(RadioSetting(f"preset{i}_band_end", "End Frequency", end_freq_rs))

            # lastUsedFreq_offset_1
            freq1 = curr_pr.lastUsedFreq_rx
            lastUsedFreq_offset_1 = RadioSettingValueInteger(0, MAX_FREQ, freq1 * 10)
            lastUsedFreq_offset_1.set_mutable(False)
            pr.append(RadioSetting(f"preset{i}_lastUsedFreq_rx", "Last Used Freq Rx", lastUsedFreq_offset_1))

            # lastUsedFreq_offset_2
            freq2 = curr_pr.lastUsedFreq_tx_msb << 5 | curr_pr.lastUsedFreq_tx_lsb
            lastUsedFreq_offset_2 = RadioSettingValueInteger(0, MAX_FREQ, freq2 * 10)
            lastUsedFreq_offset_2.set_mutable(False)
            pr.append(RadioSetting(f"preset{i}_lastUsedFreq_tx", "Last Used Freq Tx", lastUsedFreq_offset_2))

            # name
            pr.append(RadioSetting(f"preset{i}_band_name", "Band Name", RadioSettingValueString(0, 9, n)))

            # modulation
            modulation = band.modulation
            if modulation >= len(MODULATION_LIST_MAP):
                modulation = 0
            pr.append(RadioSetting(f"preset{i}_band_modulation", "Band Modulation",
                                   RadioSettingValueList(MODULATION_LIST_MAP, MODULATION_LIST_MAP[modulation])))

            # step
            step = band.step
            if step >= len(STEP_NAMES):
                step = 0
            pr.append(
                RadioSetting(f"preset{i}_band_step", "Band Step", RadioSettingValueList(STEP_NAMES, STEP_NAMES[step])))

            # bw
            bw = band.bw
            if bw >= len(BANDWIDTH_LIST):
                bw = 0
            pr.append(RadioSetting(f"preset{i}_band_bw", "Band Bandwidth",
                                   RadioSettingValueList(BANDWIDTH_LIST, BANDWIDTH_LIST[bw])))

            # squelch Type
            squelch_type = band.squelchType
            if squelch_type >= len(SQUELCH_TYPE_LIST):
                squelch_type = 0
            pr.append(RadioSetting(f"preset{i}_band_squelch_type", "Band Squelch Type",
                                   RadioSettingValueList(SQUELCH_TYPE_LIST, SQUELCH_TYPE_LIST[squelch_type])))

            # sql
            pr.append(RadioSetting(f"preset{i}_band_squelch", "SQL",
                                   RadioSettingValueInteger(0, 15, band.squelch)))

            #gainIndex
            pr.append(RadioSetting(f"preset{i}_band_gainIndex", "Gain Index",
                                   RadioSettingValueInteger(0, math.pow(2, 5) - 1, band.gainIndex)))

            # #reserved1
            # pr.append(RadioSetting("band_reserved1", "Reserved 1",
            #                        RadioSettingValueInteger(0, 255, band.reserved1)))

            #Memory Bank
            pr.append(RadioSetting(f"preset{i}_memoryBanks", "Memory Bank",
                                   RadioSettingValueInteger(0, 255, curr_pr.memoryBanks)))

            #radio
            radio = curr_pr.radio
            if radio >= len(RADIO_LIST):
                radio = 0
            pr.append(RadioSetting(f"preset{i}_radio", "Radio", RadioSettingValueList(RADIO_LIST, RADIO_LIST[radio])))

            #offset dir
            offsetDir = curr_pr.offsetDir
            if offsetDir >= len(OFFSET_DIRECTION):
                offsetDir = 0
            pr.append(RadioSetting(f"preset{i}_offsetDir", "Offset Direction",
                                   RadioSettingValueList(OFFSET_DIRECTION, OFFSET_DIRECTION[offsetDir])))

            #allow Tx
            tmpval = curr_pr.allowTx
            rs = RadioSetting(f"preset{i}_allowTx", "Allow Tx", RadioSettingValueBoolean(bool(tmpval)))
            pr.append(rs)

            #power
            power = curr_pr.power
            if power >= len(TXPOWER_LIST):
                power = 0
            pr.append(
                RadioSetting(f"preset{i}_power", "Power", RadioSettingValueList(TXPOWER_LIST, TXPOWER_LIST[power])))

            presets.append(pr)

        # is_patched = False
        # for i in range(len(_mem.Patch)):
        #     curr_patch_blocks = _mem.Patch[i].patch_data
        #     merged_blocks = 0;
        #     for j in range(len(curr_patch_blocks)):
        #         curr_patch_block = curr_patch_blocks[j]
        #         LOG.debug(f"Patch block {j} for patch {i} (== {(1 << (PATCH_BLOCK_SIZE * 4)) - 1} ? ): {curr_patch_block}")
        #         if curr_patch_block != (1 << (PATCH_BLOCK_SIZE * 4)) - 1:
        #             is_patched = True
        #             break

        # val = RadioSettingValueBoolean(is_patched)
        # val.set_mutable(False)
        # rs = RadioSetting("is_patched", "Is Patched", val)
        # patch.append(rs)

        tmpval = _mem.Settings.checkbyte
        if tmpval > 61 or tmpval < 4:
            tmpval = 4
        rs = RadioSetting("checkbyte", "Check Byte", RadioSettingValueInteger(0, 31, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.eepromType
        val = RadioSettingValueList(EEPROM_TYPE, EEPROM_TYPE[tmpval])
        val.set_mutable(False)
        rs = RadioSetting("eepromType", "EPPROM Type", val)
        basic.append(rs)

        tmpval = _mem.Settings.squelch
        rs = RadioSetting("squelch", "Squelch", RadioSettingValueInteger(0, 15, tmpval))
        sql.append(rs)

        tmpval = _mem.Settings.squelch
        rs = RadioSetting("vox", "Vox", RadioSettingValueInteger(0, 15, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.batsave
        rs = RadioSetting("battsave", "Battery Save", RadioSettingValueInteger(0, 15, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.txTime
        rs = RadioSetting("txTime", "Tx Time", RadioSettingValueInteger(0, 15, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.backlight
        try:
            tmpval_idx = BL_TIME_VALUES.index(tmpval)
        except ValueError:
            tmpval_idx = 1
        rs = RadioSetting("backlight", "BLmode (TX/RX)",
                          RadioSettingValueList(BL_TIME_NAMES,
                                                BL_TIME_NAMES[tmpval_idx]))  # transform into RadioSettingValueMap
        display.append(rs)

        tmpval = _mem.Settings.currentScanlist
        if tmpval >= 8:
            tmpval = 0

        rs = RadioSetting("currentScanlist", "Current Scan List", RadioSettingValueInteger(1, 8, tmpval + 1))
        basic.append(rs)

        tmpval = _mem.Settings.micGain
        rs = RadioSetting("micGain", "Mic Gain", RadioSettingValueInteger(0, 15, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.chDisplayMode
        rs = RadioSetting("chDisplayMode", "Channel Disply Mode", RadioSettingValueInteger(0, 3, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.scanmode
        rs = RadioSetting("scanmode", "Scan Mode", RadioSettingValueInteger(0, 3, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.roger
        rs = RadioSetting("roger", "Roger", RadioSettingValueInteger(0, 3, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.upconverter
        rs = RadioSetting("upconverter", "Upconverter",
                          RadioSettingValueList(UPCONVERTER_TYPES, UPCONVERTER_TYPES[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.dtmfdecode
        rs = RadioSetting("dtmfdecode", "DTMF Decode", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.repeaterSte
        rs = RadioSetting("repeaterSte", "repeaterSte", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.ste
        rs = RadioSetting("ste", "ste", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.busyChannelTxLock
        rs = RadioSetting("busyChannelTxLock", "Busy Channel Lock", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.keylock
        rs = RadioSetting("keylock", "Keylock", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.beep
        rs = RadioSetting("beep", "Beep", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.crossBand
        rs = RadioSetting("crossBand", "CrossBand", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.dw
        rs = RadioSetting("dw", "DW", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.contrast
        rs = RadioSetting("contrast", "Contrast", RadioSettingValueInteger(0, 15, tmpval))
        display.append(rs)

        tmpval = _mem.Settings.brightness
        rs = RadioSetting("brightness", "Brightness", RadioSettingValueInteger(0, 15, tmpval))
        display.append(rs)

        tmpval = _mem.Settings.mainApp
        rs = RadioSetting("mainApp", "Main App", RadioSettingValueList(APP_LIST, APP_LIST[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.activePreset
        PRESET_NAMES = [re.sub(ALLOWED_CHARS_PATTERN, ' ', str(preset.Band.name)).strip() + "\x00" for preset in
                        _mem.Preset]
        LOG.debug(f"PRESET_NAMES: {PRESET_NAMES}, activePreset: {tmpval}")
        rs = RadioSetting("activePreset", "Active Preset", RadioSettingValueList(PRESET_NAMES, PRESET_NAMES[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.presetsCount
        val = RadioSettingValueInteger(0, 255, tmpval)
        val.set_mutable(False)
        rs = RadioSetting("presetsCount", "Preset Count", val)
        basic.append(rs)

        #Battery
        tmpval = _mem.Settings.batteryStyle
        rs = RadioSetting("batteryStyle", "Battery Style Name",
                          RadioSettingValueList(BATTERY_STYLE_NAMES, BATTERY_STYLE_NAMES[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.batteryType
        rs = RadioSetting("batteryType", "Battery Type",
                          RadioSettingValueList(BATTERY_TYPE_NAMES, BATTERY_TYPE_NAMES[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.batteryCalibration
        rs = RadioSetting("batteryCalibration", "Battery Calibration",
                          RadioSettingValueInteger(0, math.pow(2, 12) - 1, tmpval))
        basic.append(rs)

        tmpval = _mem.Settings.sqOpenedTimeout
        rs = RadioSetting("sqOpenedTimeout", "SQL Opened Timeout",
                          RadioSettingValueList(SCAN_TIMEOUT_NAMES, SCAN_TIMEOUT_NAMES[tmpval]))
        sql.append(rs)

        tmpval = _mem.Settings.sqClosedTimeout
        rs = RadioSetting("sqClosedTimeout", "SQL Closed Timeout",
                          RadioSettingValueList(SCAN_TIMEOUT_NAMES, SCAN_TIMEOUT_NAMES[tmpval]))
        sql.append(rs)

        tmpval = _mem.Settings.backlightOnSquelch
        rs = RadioSetting("backlightOnSquelch", "Backlight On Squelch",
                          RadioSettingValueList(BL_SQL_MODE_NAMES, BL_SQL_MODE_NAMES[tmpval]))
        display.append(rs)

        tmpval = _mem.Settings.noListen
        rs = RadioSetting("noListen", "No Listen", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.bound_240_280
        rs = RadioSetting("bound_240_280", "Bound 240 / 280",
                          RadioSettingValueList(BOUND_240_280_NAMES, BOUND_240_280_NAMES[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.scanTimeout
        if tmpval >= len(SCAN_TIMEOUT):
            tmpval = 0
        rs = RadioSetting("scanTimeout", "Scan Timeout", RadioSettingValueList(SCAN_TIMEOUT, SCAN_TIMEOUT[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.activeVFO
        rs = RadioSetting("activeVFO", "Active VFO", RadioSettingValueList(VFOs, VFOs[tmpval]))
        basic.append(rs)

        tmpval = _mem.Settings.skipGarbageFrequencies
        rs = RadioSetting("skipGarbageFrequencies", "Skip Garbage Frequencies", RadioSettingValueBoolean(bool(tmpval)))
        basic.append(rs)

        tmpval = _mem.Settings.sqlOpenTime
        if tmpval >= len(SQL_OPEN_NAMES):
            tmpval = 0
        rs = RadioSetting("sqlOpenTime", "Sql Open Time", RadioSettingValueList(SQL_OPEN_NAMES, SQL_OPEN_NAMES[tmpval]))
        sql.append(rs)

        tmpval = _mem.Settings.sqlCloseTime
        if tmpval >= len(SQL_CLOSE_NAMES):
            tmpval = 0
        rs = RadioSetting("sqlCloseTime", "Sql Close Time",
                          RadioSettingValueList(SQL_CLOSE_NAMES, SQL_CLOSE_NAMES[tmpval]))
        sql.append(rs)

        tmpval = sanitize_str(_mem.Settings.nickName)
        rs = RadioSetting("nickName", "Nick Name", RadioSettingValueString(0, 10, str(tmpval).strip("\x20\x00\xff")))
        basic.append(rs)

        top = RadioSettings(basic, display, sql, patch, *vfo, *presets)
        return top

    # --------------------------------------------------------------------------------
    def set_settings(self, settings):

        _mem = self._memobj
        for element in settings:
            if not isinstance(element, RadioSetting):
                self.set_settings(element)
                continue

            if element.get_name() == "checkbyte":
                _mem.Settings.checkbyte = int(element.value)

            if element.get_name() == "eepromType":
                _mem.Settings.eepromType = EEPROM_TYPE.index(element.value)

            if element.get_name() == "squelch":
                _mem.Settings.squelch = int(element.value)

            if element.get_name() == "vox":
                _mem.Settings.vox = int(element.value)

            if element.get_name() == "batsave":
                _mem.Settings.batsave = int(element.value)

            if element.get_name() == "txTime":
                _mem.Settings.txTime = int(element.value)

            if element.get_name() == "backlight":
                _mem.Settings.backlight = BL_TIME_VALUES[BL_TIME_NAMES.index(element.value)]

            if element.get_name() == "currentScanlist":
                _mem.Settings.currentScanlist = int(element.value) - 1

            if element.get_name() == "micGain":
                _mem.Settings.micGain = int(element.value)

            if element.get_name() == "chDisplayMode":
                _mem.Settings.chDisplayMode = int(element.value)

            if element.get_name() == "scanmode":
                _mem.Settings.scanmode = int(element.value)

            if element.get_name() == "roger":
                _mem.Settings.roger = int(element.value)

            if element.get_name() == "upconverter":
                _mem.Settings.upconverter = UPCONVERTER_TYPES.index(element.value)

            if element.get_name() == "dtmfdecode":
                _mem.Settings.dtmfdecode = element.value and 1 or 0

            if element.get_name() == "repeaterSte":
                _mem.Settings.repeaterSte = element.value and 1 or 0

            if element.get_name() == "ste":
                _mem.Settings.ste = element.value and 1 or 0

            if element.get_name() == "busyChannelTxLock":
                _mem.Settings.busyChannelTxLock = element.value and 1 or 0

            if element.get_name() == "keylock":
                _mem.Settings.keylock = element.value and 1 or 0

            if element.get_name() == "beep":
                _mem.Settings.beep = element.value and 1 or 0

            if element.get_name() == "crossBand":
                _mem.Settings.crossBand = element.value and 1 or 0

            if element.get_name() == "dw":
                _mem.Settings.dw = element.value and 1 or 0

            if element.get_name() == "contrast":
                _mem.Settings.contrast = int(element.value)

            if element.get_name() == "brightness":
                _mem.Settings.brightness = int(element.value)

            if element.get_name() == "mainApp":
                _mem.Settings.mainApp = APP_LIST.index(element.value)

            if element.get_name() == "activePreset":
                # Probably this is not the best way to do it; but it works if not changning the preset.Band.name
                PRESET_NAMES = [re.sub(ALLOWED_CHARS_PATTERN, ' ', str(preset.Band.name)).strip() + "\x00" for preset in
                                _mem.Preset]

                _mem.Settings.activePreset = PRESET_NAMES.index(element.value)

            if element.get_name() == "batteryStyle":
                _mem.Settings.batteryStyle = BATTERY_STYLE_NAMES.index(element.value)

            if element.get_name() == "batteryType":
                _mem.Settings.batteryType = BATTERY_TYPE_NAMES.index(element.value)

            if element.get_name() == "batteryCalibration":
                _mem.Settings.batteryCalibration = int(element.value)

            if element.get_name() == "sqClosedTimeout":
                _mem.Settings.sqClosedTimeout = SCAN_TIMEOUT_NAMES.index(element.value)

            if element.get_name() == "sqOpenedTimeout":
                _mem.Settings.sqOpenedTimeout = SCAN_TIMEOUT_NAMES.index(element.value)

            if element.get_name() == "backlightOnSquelch":
                _mem.Settings.backlightOnSquelch = BL_SQL_MODE_NAMES.index(element.value)

            if element.get_name() == "noListen":
                _mem.Settings.noListen = element.value and 1 or 0

            if element.get_name() == "bound_240_280":
                _mem.Settings.bound_240_280 = BOUND_240_280_NAMES.index(element.value)

            if element.get_name() == "scanTimeout":
                _mem.Settings.scanTimeout = SCAN_TIMEOUT.index(element.value)

            if element.get_name() == "activeVFO":
                _mem.Settings.activeVFO = VFOs.index(element.value)

            if element.get_name() == "skipGarbageFrequencies":
                _mem.Settings.skipGarbageFrequencies = element.value and 1 or 0

            if element.get_name() == "sqlCloseTime":
                _mem.Settings.sqlCloseTime = SQL_CLOSE_NAMES.index(element.value)

            if element.get_name() == "sqlOpenTime":
                _mem.Settings.sqlOpenTime = SQL_OPEN_NAMES.index(element.value)

            if element.get_name() == "nickName":
                _mem.Settings.nickName = element.value

            # VFO
            reg_patt = "^vfo(([0-9]){1,2})_"
            if re.search(reg_patt, element.get_name()):
                vfo_idx = int(re.search(reg_patt, element.get_name()).group(1))
                base_vfo_name = f"vfo{vfo_idx}"

                if element.get_name() == f"{base_vfo_name}_tx_f":
                    _mem.VFO[vfo_idx].tx.f = int(element.value) / 10

                if element.get_name() == f"{base_vfo_name}_tx_codeType":
                    #  Needed some logic to update the settings list values
                    _mem.VFO[vfo_idx].tx.codeType = TX_CODE_TYPES.index(element.value)
                    # LOG.debug(f"TX_CODE_TYPES: {TX_CODE_TYPES}, element.value: {element }")
                    # #list all keys in self object
                    # LOG.debug(dir(self))
                    # self.get_settings()
                    # set_tx_code(_mem, _mem.VFO[vfo_idx], vfo_idx, element)

                if element.get_name() == f"{base_vfo_name}_tx_code":
                    _mem.VFO[vfo_idx].tx.code = int(element.value)

                if element.get_name() == f"{base_vfo_name}_rx_f":
                    _mem.VFO[vfo_idx].rx.f = int(element.value) / 10

                if element.get_name() == f"{base_vfo_name}_rx_codeType":
                    _mem.VFO[vfo_idx].rx.codeType = RX_CODE_TYPES.index(element.value)

                if element.get_name() == f"{base_vfo_name}_rx_code":
                    _mem.VFO[vfo_idx].rx.code = int(element.value)
                    set_rx_code(_mem, _mem.VFO[vfo_idx], vfo_idx, element)

                if element.get_name() == f"{base_vfo_name}_channel":
                    _mem.VFO[vfo_idx].channel = int(element.value) - 1

                if element.get_name() == f"{base_vfo_name}_modulation":
                    _mem.VFO[vfo_idx].modulation = MODULATION_LIST_MAP.index(element.value)

                if element.get_name() == f"{base_vfo_name}_power":
                    _mem.VFO[vfo_idx].power = TX_POWER_NAMES.index(element.value)

                if element.get_name() == f"{base_vfo_name}_radio":
                    _mem.VFO[vfo_idx].radio = RADIO_LIST.index(element.value)

            # Preset
            reg_patt = "^preset(([0-9]){1,2})_"
            if re.search(reg_patt, element.get_name()):
                pr_idx = int(re.search(reg_patt, element.get_name()).group(1))
                base_pr_name = f"preset{pr_idx}"

                if element.get_name() == f"{base_pr_name}_PowerCalibration_s":
                    _mem.Preset[pr_idx].PowerCalibration.s = int(element.value)

                if element.get_name() == f"{base_pr_name}_PowerCalibration_m":
                    _mem.Preset[pr_idx].PowerCalibration.m = int(element.value)

                if element.get_name() == f"{base_pr_name}_PowerCalibration_e":
                    _mem.Preset[pr_idx].PowerCalibration.e = int(element.value)

                if element.get_name() == f"{base_pr_name}_band_start":
                    _mem.Preset[pr_idx].Band.FRange.start = int(element.value) / 10

                if element.get_name() == f"{base_pr_name}_band_end":
                    end = int(int(element.value) / 10)
                    _mem.Preset[pr_idx].Band.FRange.end_msb = (end >> 5) & 0x3FFFFF
                    _mem.Preset[pr_idx].Band.FRange.end_lsb = end & 0x1FF

                    LOG.debug(
                        f"end: {end}, _mem.Preset[pr_idx].Band.FRange.end_msb: {_mem.Preset[pr_idx].Band.FRange.end_msb}, _mem.Preset[pr_idx].Band.FRange.end_lsb: {_mem.Preset[pr_idx].Band.FRange.end_lsb}")

                if element.get_name() == f"{base_pr_name}_band_name":
                    _mem.Preset[pr_idx].Band.name = sanitize_str_10(element.value)

                if element.get_name() == f"{base_pr_name}_band_modulation":
                    _mem.Preset[pr_idx].Band.modulation = MODULATION_LIST_MAP.index(element.value)

                if element.get_name() == f"{base_pr_name}_band_step":
                    _mem.Preset[pr_idx].Band.step = STEP_NAMES.index(element.value)

                if element.get_name() == f"{base_pr_name}_band_bw":
                    _mem.Preset[pr_idx].Band.bw = BANDWIDTH_LIST.index(element.value)

                if element.get_name() == f"{base_pr_name}_band_squelch_type":
                    _mem.Preset[pr_idx].Band.squelchType = SQUELCH_TYPE_LIST.index(element.value)

                if element.get_name() == f"{base_pr_name}_band_squelch":
                    _mem.Preset[pr_idx].Band.squelch = int(element.value)

                if element.get_name() == f"{base_pr_name}_band_gainIndex":
                    _mem.Preset[pr_idx].Band.gainIndex = int(element.value)

                if element.get_name() == f"{base_pr_name}_memoryBanks":
                    _mem.Preset[pr_idx].memoryBanks = int(element.value)

                if element.get_name() == f"{base_pr_name}_radio":
                    _mem.Preset[pr_idx].radio = RADIO_LIST.index(element.value)

                if element.get_name() == f"{base_pr_name}_offsetDir":
                    _mem.Preset[pr_idx].offsetDir = OFFSET_DIRECTION.index(element.value)

                if element.get_name() == f"{base_pr_name}_allowTx":
                    _mem.Preset[pr_idx].allowTx = element.value and 1 or 0

                if element.get_name() == f"{base_pr_name}_power":
                    _mem.Preset[pr_idx].power = TXPOWER_LIST.index(element.value)
