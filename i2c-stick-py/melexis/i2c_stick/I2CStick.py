import serial
import time

class I2CStick:
    ser = None
    def __init__(self, port):
        self.open(port)

    def open(self, port):
        self.ser = serial.Serial(port, 921600, timeout=5)
        self.ser.write(b'!')    
        time.sleep(0.2)
        self.ser.flushInput()
        self.ser.flushOutput()
        time.sleep(0.1)

    def close(self):
        if self.ser is not None:
            self.ser.close()
        self.ser = None

    def mv(self, sa):
        self.ser.write('mv:{:02X}'.format(sa).encode()+ b'\n')
        line = self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line
        values = line.split(":")[-1]     
        return [float(value) for value in values.split(",")]
    
    def read_continuous_message(self):
        line = self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line
        if line.startswith("@"):
            values = line.split(":")
            if values[2] == 'mv':
                mv_list = [float(v) for v in values[-1].split(",")]
                return {'sa': int(values[-3], 16), 'time': int(values[-2]), 'drv': int(values[1], 16), 'mv': mv_list}
        return None

    def stop_continuous_mode(self):
        self.ser.write(b'!')
        time.sleep(0.1)
        self.ser.flushInput()
        self.ser.flushOutput()
        time.sleep(0.1)

    def trigger_continuous_mode(self):
        self.ser.write(b'!')
        time.sleep(0.1)
        self.ser.flushInput()
        self.ser.flushOutput()
        self.ser.write(b';')
        self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line

    def run_cmd(self, cmd):
        self.ser.write(bytes(cmd+"\n", 'utf-8'))
        return self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line

    def pwm(self, pin, pwm):
        a = self.run_cmd("pwm:{}:{}".format(pin, pwm))
        return a

    def sn(self, sa):
        a = self.run_cmd("sn:{:02X}".format(sa))
        a = a.split(":")
        if a[0] != "sn":
            return None
        if a[1] != "{:02X}".format(sa):
            return None
        return a[2]


    def mr(self, sa, address, count):
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
        if type(data) == list:
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

    def mlx(self):
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
            a = self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result


    def fv(self):
        a = self.run_cmd("fv")
        a = a.split(":")
        if a[0] != "fv":
            return None
        return a[1]


    def bi(self):
        a = self.run_cmd("bi")
        a = a.split(":")
        if a[0] != "bi":
            return None
        return a[1]


    def ls(self):
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
            a = self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result


    def scan(self):
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
            a = self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line
            t = time.time() - start

        self.ser.timeout = timeout_old

        return result


if (__name__ == '__main__'):
    mis = I2CStick("COM154")
    print(mis.mv(0x3A))
    mis.trigger_continuous_mode()
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    print(mis.read_continuous_message())
    mis.stop_continuous_mode()
