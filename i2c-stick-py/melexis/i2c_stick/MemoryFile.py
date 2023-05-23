import bincopy
import struct

# goal is to extend the functionality and update the default creation parameter of a BinFile object.

class MemoryFile(bincopy.BinFile):
    def __init__(self, filenames=None, overwrite=True, header_encoding='utf-8', bytes_per_address=2):
        super().__init__(filenames=filenames, overwrite=overwrite, header_encoding=header_encoding)
        '''
            note this parameter 'bytes_per_address' is not the same as the bincopy's 'word_size_bits'.
            That parameter ('word_size_bits') would create hex files which are not readable by all other tools out there to read a hex file.
            Instead, here we internally use an address which links one address to one single byte.
            So for many melexis products this means the address you find in the hex-file is double of the actual address to be used when writing.
            bytes_per_address aims to ease that process, however when using other libraries to parse the hexfiles, one have to understand that the address requires to be divided by two.
        '''
        self.bytes_per_address = bytes_per_address


    @staticmethod
    def read_evb90614_32_40_41_bin_file(evb_bin_file, at_address):
        # EVB90614, EVB90632, EVB90640-41 BIN file format is little-endian, while bin_copy; the hex file package expects big-endian!
        # so here we do the trick with unpack/pack!
        self.bytes_per_address = 2 # force 2 bytes per address for MLX90632!
        with open(evb_bin_file, mode='rb') as file: # b is important -> binary
            fileContent = file.read()

        fmt_in = "<{:d}H".format (int(len(fileContent)/2))
        fmt_out = ">{:d}H".format (int(len(fileContent)/2))
        data = list(struct.unpack(fmt_in, fileContent))
        bin_data = struct.pack(fmt_out, *data)

        bin_file = MemoryFile()
        bin_file.add_binary(bin_data, address=at_address*self.bytes_per_address, overwrite=True)
        return bin_file


    def add_32(self, address, int_data):
        if type(int_data) is list:
            byte_array = struct.pack('>{}L'.format(len(int_data)), *int_data)
        else:
            byte_array = struct.pack('>L', int_data)
        self.add_binary(byte_array, address=address, overwrite=True)


    def add_32s(self, address, int_data):
        if type(int_data) is list:
            byte_array = struct.pack('>{}l'.format(len(int_data)), *int_data)
        else:
            byte_array = struct.pack('>l', int_data)
        self.add_binary(byte_array, address=address, overwrite=True)


    def add_32_swapped(self, address, int_data):
        if type(int_data) is list:
            byte_array = struct.pack('>{}L'.format(len(int_data)), *int_data)
        else:
            byte_array = struct.pack('>L', int_data)
        # silly mistake for 90632 in the eeprom map... LWORD is before HWORD
        for i in range(0, len(byte_array), 4):
            self.add_binary(byte_array[i+2:i+4], address=address*self.bytes_per_address, overwrite=True)
            self.add_binary(byte_array[i+0:i+2], address=(address+1)*self.bytes_per_address, overwrite=True)


    def add_32s_swapped(self, address, int_data):
        if type(int_data) is list:
            byte_array = struct.pack('>{}l'.format(len(int_data)), *int_data)
        else:
            byte_array = struct.pack('>l', int_data)
        # silly mistake for 90632 in the eeprom map... LWORD is before HWORD
        for i in range(0, len(byte_array), 4):
            self.add_binary(byte_array[i+2:i+4], address=address*self.bytes_per_address, overwrite=True)
            self.add_binary(byte_array[i+0:i+2], address=(address+1)*self.bytes_per_address, overwrite=True)


    def add_16(self, address, int_data):
        if type(int_data) is list:
            byte_array = struct.pack('>{}H'.format(len(int_data)), *int_data)
        else:
            byte_array = struct.pack('>H', int_data)
        self.add_binary(byte_array, address=address*self.bytes_per_address, overwrite=True)


    def add_16s(self, address, int_data):
        if type(int_data) is list:
            byte_array = struct.pack('>{}h'.format(len(int_data)), *int_data)
        else:
            byte_array = struct.pack('>h', int_data)
        self.add_binary(byte_array, address=address*self.bytes_per_address, overwrite=True)


    def write_evb90614_32_40_41_bin_file(self, address, count, evb_bin_file_name = "evb.bin"):
        if evb_bin_file_name is None:
            return None

        if not evb_bin_file_name.endswith('.bin'):
            evb_bin_file_name += '.bin'

        bin_data_in = self[address*self.bytes_per_address:(address+count)*self.bytes_per_address].as_binary()
        if len(bin_data_in) < (count*self.bytes_per_address):
            bin_data_in.append(bytearray([255])*((count*self.bytes_per_address)-len(bin_data_in)))
        fmt_in = ">{:d}H".format (int(len(bin_data_in)/2))
        fmt_out = "<{:d}H".format (int(len(bin_data_in)/2))

        data = list(struct.unpack(fmt_in, bin_data_in))
        bin_data = struct.pack(fmt_out, *data)

        with open(evb_bin_file_name, "wb") as out_file:
            out_file.write(bin_data)

        return evb_bin_file_name


    def write_hex_file(self, hex_file_name = "memory_file.hex"):
        if hex_file_name is None:
            return self.as_ihex()

        if not hex_file_name.endswith('.hex'):
            hex_file_name += '.hex'

        with open(hex_file_name, "w") as out_file:
            out_file.write(self.as_ihex())

        return hex_file_name


    def get_address_data_pairs(self):
        pairs = []
        for seg in self.segments:
            for addr in range(0, int((seg.maximum_address-seg.minimum_address) / seg._word_size_bytes), self.bytes_per_address):
                addr += int(seg.minimum_address / seg._word_size_bytes)
                data = 0
                for i in range(self.bytes_per_address):
                    data <<= 8
                    data += (int(self[addr+i]))

                pairs.append ((int(addr/self.bytes_per_address), data))
        return pairs
