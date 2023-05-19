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

    def set_pwm(self, pin, pwm):
        self.ser.write(bytes('pwm:{}:{}\n'.format(pin, pwm), 'utf-8'))
        return self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line

    def run_cmd(self, cmd):
        self.ser.write(bytes(cmd+"\n", 'utf-8'))
        return self.ser.readline().decode('utf-8').rstrip()   # read a '\n' terminated line


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
