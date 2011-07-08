from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option
import rfidbts_swig as rfidbts

from bitarray import bitarray
from crc_algorithms import Crc

def string_xor(str1, str2):
    out = ""
    for i in range(len(str1)):
        if str1[i] != str2[i]:
            out = out[:(i)] + '1' + out[(i+1):]
        else:
            out = out[:(i)] + '0' + out[(i+1):]   
    return out

def make3_crc_5(bit_stream):
    preset = "01001"
    register = "01001"
    flag = True
    for i in range(len(bit_stream)):
        if register[0] == bit_stream[0]:
            flag = False
        else:
            flag = True
        register = register[1:] + "0"
        bit_stream = bit_stream[1:]
        if flag:
            register = string_xor(register, preset)
    return register

