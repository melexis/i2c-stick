import serial
import time
import re


class I2CStick:
    ser = None

    def __init__(self, port):
        self.open(port)

    def open(self, port):
        """Open the PC connection to the I2C-stick"""
        self.ser = serial.Serial(port, 921600, timeout=5)
        self.ser.write(b'!')
        time.sleep(0.2)
        self.ser.flushInput()
        self.ser.flushOutput()
        time.sleep(0.1)

    def close(self):
        """Close the PC connection to the I2C-stick"""
        if self.ser is not None:
            self.ser.close()
        self.ser = None

    def read_continuous_message(self):
        """Read the message from continuous mode"""
        line = self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line
        if line.startswith("@"):
            values = line.split(":")
            if values[2] == 'mv':
                mv_list = [float(v) for v in values[-1].split(",")]
                return {'sa': int(values[-3], 16), 'time': int(values[-2]), 'drv': int(values[1], 16), 'mv': mv_list}
        return None

    def stop_continuous_mode(self):
        """Stop continuous mode"""
        self.ser.write(b'!')
        time.sleep(0.1)
        self.ser.flushInput()
        self.ser.flushOutput()
        time.sleep(0.1)

    def trigger_continuous_mode(self):
        """Start continuous mode"""
        self.ser.write(b'!')
        time.sleep(0.1)
        self.ser.flushInput()
        self.ser.flushOutput()
        self.ser.write(b';')
        self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line

    def run_cmd(self, cmd):
        """Generic run command"""
        self.ser.write(bytes(cmd + "\n", 'utf-8'))
        return self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line

    def mlx(self):
        """MeLeXis test command"""
        result = []
        timeout_old = self.ser.timeout
        self.ser.timeout = 0.25

        a = self.run_cmd("mlx")
        t = 0
        while t < 0.1:
            a = a.split(":")
            if a[0] != "mlx":
                self.ser.timeout = timeout_old
                return None
            result.append(a[1])
            start = time.time()
            a = self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result

    def fv(self):
        """Firmware Version command"""
        a = self.run_cmd("fv")
        a = a.split(":")
        if a[0] != "fv":
            return None
        return a[1]

    def firmware_version(self):
        return self.fv()

    def bi(self):
        """get Board Information"""
        a = self.run_cmd("bi")
        a = a.split(":")
        if a[0] != "bi":
            return None
        return a[1]

    def i2c_read(self, sa):
        return None

    def i2c_write(self, sa):
        return None

    def i2c_addressed_read(self, sa):
        return None

    def ch(self):
        """get Configure Host command"""
        result = {}
        timeout_old = self.ser.timeout
        self.ser.timeout = 0.25

        a = self.run_cmd("ch")
        t = 0
        while t < 0.1:
            a = a.split(":")
            if a[0] != "ch":
                self.ser.timeout = timeout_old
                return None
            else:
                # regex ==> https://regex101.com/r/ltml2J/1
                regex = r"(?P<key>\S+)=(?P<value>[^\s\(\)]+)(?:\((?P<description>\S+)\))?"
                r = re.match(regex, a[1])
                item = r.groupdict()
                key = item['key']
                if key not in result.keys():
                    result[key] = []
                item.pop('key')
                if key == "SA_DRV":
                    new_item = {}
                    sa, drv, product = item['value'].split(',')
                    new_item['SA'] = int(sa, 16)
                    new_item['DRV'] = int(drv, 16)
                    new_item['product'] = product
                    result[key].append(new_item)
                elif key == "DRV":
                    new_item = {}
                    drv, product = item['value'].split(',')
                    new_item['DRV'] = int(drv, 16)
                    new_item['product'] = product
                    result[key].append(new_item)
                else:
                    result[key].append(item)

            start = time.time()
            a = self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result

    def ch_write(self, item, value):
        """set the Configuration of the Host(I2C-stick)"""
        """
        +ch:I2C_FREQ=F100k
        2023/11/01 17:16:01.613 ->     - +ch:I2C_FREQ=F400k
        2023/11/01 17:16:01.613 ->     - +ch:I2C_FREQ=F1M
        2023/11/01 17:16:01.613 ->     - +ch:I2C_FREQ=F50k
        2023/11/01 17:16:01.613 ->     - +ch:I2C_FREQ=F20k
        2023/11/01 17:16:01.613 ->     - +ch:I2C_FREQ=F10k
        
        """
        str_value = str(value)
        if item == "I2C_FREQ":
            items = {
                10000: 'F10k',
                20000: 'F20k',
                50000: 'F50k',
                100000: 'F100k',
                400000: 'F400k',
                1000000: 'F1M',
            }
            if value not in items.keys():
                return "value not valid"
            str_value = items[value]
        a = self.run_cmd("+ch:{}={}".format(item, str_value))
        a = a.split(":")
        # +ch:OK [host-register]
        # +ch:I2C_FREQ=F2M:ERROR: Invalid value
        if a[0] != "+ch":
            return "wrong command returned"
        if a[1].startswith("OK"):
            return
        else:
            return a[1]

    def scan(self):
        """SCAN the i2c bus for slaves"""
        result = []
        timeout_old = self.ser.timeout
        self.ser.timeout = 0.25

        a = self.run_cmd("scan")
        t = 0
        while t < 0.1:
            a = a.split(":")
            if a[0] == "scan":
                if len(a) < 3:
                    result.append(a[1])
                else:
                    item = {}
                    item['sa'] = int(a[1], 16)
                    a = a[2].split(',')
                    item['drv'] = int(a[0], 16)
                    item['raw'] = int(a[1], 16)
                    item['disabled'] = int(a[2], 16)
                    item['product'] = a[3]
                    result.append(item)
            start = time.time()
            a = self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result

    def ls(self):
        """List Slaves which are previously found by scan command"""
        result = []
        timeout_old = self.ser.timeout
        self.ser.timeout = 0.25

        a = self.run_cmd("ls")
        t = 0
        while t < 0.1:
            a = a.split(":")
            if a[0] == "ls":
                if len(a) < 3:
                    result.append(a[1])
                else:
                    item = {}
                    item['sa'] = int(a[1], 16)
                    a = a[2].split(',')
                    item['drv'] = int(a[0], 16)
                    item['raw'] = int(a[1], 16)
                    item['disabled'] = int(a[2], 16)
                    item['product'] = a[3]
                    result.append(item)
            start = time.time()
            a = self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result

    def dis(self, sa, disable=1):
        """DISable a slave from continuous mode"""
        a = self.run_cmd("dis:{:02X}:{}".format(sa, disable))
        a = a.split(":")
        if a[0] != "dis":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        if a[2] == "FAIL":
            return "FAIL:" + a[3]
        result = {'status': a[3],
                  'disabled': int(a[2])}
        return result

    def sn(self, sa):
        """read the Serial Number of the slave"""
        a = self.run_cmd("sn:{:02X}".format(sa))
        a = a.split(":")
        if a[0] != "sn":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        return a[2]

    def cs(self, sa):
        """read the Configuration of the Slave"""
        result = {}
        timeout_old = self.ser.timeout
        self.ser.timeout = 0.25

        a = self.run_cmd("cs:{:02X}".format(sa))
        t = 0
        while t < 0.1:
            a = a.split(":")
            if a[0] != "cs":
                self.ser.timeout = timeout_old
                return None
            if a[1] != "{:02X}".format(sa):
                self.ser.timeout = timeout_old
                return None
            if a[2] == "RO":
                if 'read_only' not in result.keys():
                    result['read_only'] = {}
                a = a[3].split("=")
                value = a[1].split(",")
                for i, v in enumerate(value):
                    regex = r"(?P<value>[^\s\(\)]+)(?:\((?P<description>\S+)\))?"
                    new_value = re.match(regex, v).groupdict()
                    if (new_value['value'][:1].isdigit() or new_value['value'][:1] == '-') & (a[0] != 'SA'):
                        try:
                            new_value['value'] = int(new_value['value'])
                        except:
                            try:
                                new_value['value'] = float(new_value['value'])
                            except:
                                None
                            None
                    if new_value['description'] is not None:
                        value[i] = new_value
                    else:
                        value[i] = new_value['value']
                if len(value) == 1:
                    value = value[0]
                result['read_only'][a[0]] = value
            else:
                a = a[2].split("=")
                value = a[1].split(",")
                for i, v in enumerate(value):
                    regex = r"(?P<value>[^\s\(\)]+)(?:\((?P<description>\S+)\))?"
                    new_value = re.match(regex, v).groupdict()
                    if (new_value['value'][:1].isdigit() or new_value['value'][:1] == '-') & (a[0] != 'SA'):
                        try:
                            new_value['value'] = int(new_value['value'])
                        except:
                            try:
                                new_value['value'] = float(new_value['value'])
                            except:
                                None
                            None
                    if new_value['description'] is not None:
                        value[i] = new_value
                    else:
                        value[i] = new_value['value']
                if len(value) == 1:
                    value = value[0]
                result[a[0]] = value
            start = time.time()
            a = self.ser.readline().decode('utf-8').rstrip()  # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result

    def cs_write(self, sa, item, value):
        """write the Configuration of the Slave"""
        a = self.run_cmd("+cs:{:02X}:{}={}".format(sa, item, value))
        a = a.split(":")
        # +cs:3A:MEAS_SELECT=OK [host&mlx-register]'
        # '+cs:3A:FAIL; unknown variable'
        if a[0] != "+cs":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        if a[2].startswith("FAIL"):
            return a[2]
        return a[2]

    def nd(self, sa):
        """check if New Data is available for the slave"""
        a = self.run_cmd("nd:{:02X}".format(sa))
        a = a.split(":")
        if a[0] != "nd":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        if a[2] == "FAIL":
            return "FAIL:" + a[3]
        return int(a[2])

    def mr(self, sa, address, count):
        """Memory Read from the slave"""
        a = self.run_cmd("mr:{:02X}:{:04X},{:04X}".format(sa, address, count))
        a = a.split(":")
        if a[0] != "mr":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        (addr, bits_in_data, address_increments, address_count, d, data) = a[2].split(',', 5)
        if int(addr, 16) != address:
            return None
        if d != 'DATA':
            return None
        addr = int(addr, 16)
        bits_in_data = int(bits_in_data, 16)
        address_increments = int(address_increments, 16)
        address_count = int(address_count, 16)
        bytes_per_address = int(bits_in_data / 8 / address_increments)
        result = {}
        result['sa'] = sa
        result['address'] = addr
        result['bits_in_data'] = bits_in_data
        result['address_increments'] = address_increments
        result['address_count'] = address_count
        result['bytes_per_address'] = bytes_per_address
        result['data'] = [int(x, 16) for x in data.split(',')]

        return result

    def mw(self, sa, address, data):
        """Memory Write to the slave"""
        if type(data) is list:
            data_str = ",".join(["{:04X}".format(x) for x in data])
        else:
            data_str = "{:04X}".format(data)
        a = self.run_cmd("mw:{:02X}:{:04X},{}".format(sa, address, data_str))
        a = a.split(":")
        if a[0] != "mw":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        return a[2]

    def mv(self, sa):
        """Measure Values from slave sensor"""
        a = self.run_cmd("mv:{:02X}".format(sa))
        a = a.split(":")
        if a[0] != "mv":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        result = {}
        result['time_ms'] = int(a[2])
        result['values'] = [float(x) for x in a[3].split(',')]
        return result

    def raw(self, sa):
        """measure RAW values from slave sensor"""
        a = self.run_cmd("raw:{:02X}".format(sa))
        a = a.split(":")
        if a[0] != "raw":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        if a[2] == "FAIL":
            return "FAIL:" + a[3]
        result = {}
        result['time_ms'] = int(a[2])
        result['values'] = [int(x, 16) for x in a[3].split(',')]
        result['values'] = [x if x < 2 ** 15 else x - 2 ** 16 for x in result['values']]
        return result

    def pwm(self, pin, pwm):
        """set a PWM duty cycle on a pin of the I2C-stick"""
        a = self.run_cmd("pwm:{}:{}".format(pin, pwm))
        return a

    def pinvalue(self, pin, val): # alias for pin
        return self.pin(pin, val)

    def pin(self, pin, val=None):
        """set or get a pin of the I2C-stick"""
        if val is None:
            return self.run_cmd("pin:{}".format(pin))
        if val == 'pullup':
            return self.run_cmd("pin:{}+".format(pin))
        return self.run_cmd("pin:{}:{}".format(pin, val))


if __name__ == '__main__':
    mis = I2CStick("COM154")
    print(mis.mv(0x3A))
    mis.trigger_continuous_mode()
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    mis.stop_continuous_mode()
