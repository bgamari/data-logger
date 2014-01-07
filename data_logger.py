from serial import Serial
import logging

class DataLogger(object):
    def __init__(self, device=None):
        if device == None:
            # TODO: Look for device more intelligently
            device = '/dev/ttyACM0'

        self._dev = Serial(device, baudrate=115200)

    def _write_cmd(self, cmd):
        logging.debug('command: %s' % cmd)
        self._dev.write(cmd + '\n')
        
    def _read_reply(self):
        while True:
            l = self._dev.readline().strip()
            if len(l.strip()) == 0:
                return
            else:
                logging.debug('reply: %s' % l)
                yield l

    def _read_single_reply(self):
        """ Read a single-line reply """
        reply = list(self._read_reply())
        if len(reply) != 1:
            raise RuntimeError('invalid reply: %s' % reply)
        return reply[0]

    def _read_reply_value(self):
        reply = self._read_single_reply()
        return reply.split('=')[1].strip()

    def get_firmware_version(self):
        self._write_cmd('V')
        return self._read_reply_value()

    def get_device_id(self):
        self._write_cmd('I')
        return self._read_reply_value()

    def set_verbose(self, verbose):
        self._write_cmd('v=%d' % verbose)
        return bool(int(self._read_reply_value()))
        
    def get_sample_count(self):
        self._write_cmd('n')
        return int(self._read_reply_value())

    def _parse_sample(self, line):
        l = line.split()
        if len(l) != 3:
            raise RuntimeError("Invalid sample line")
        time = int(l[0])
        sensor = int(l[1])
        value = float(l[2])
        return (time, sensor, value)

    def fetch_samples(self, start, count):
        self._write_cmd('g %d %d' % (start, count))
        for l in self._read_reply():
            yield self._parse_sample(l)

    def set_acquisition_state(self, running):
        self._write_cmd('a=%d' % running)
        return bool(int(self._read_reply_value()))

    def get_acquisition_state(self):
        self._write_cmd('a')
        return bool(int(self._read_reply_value()))

    def identify_flash(self):
        self._write_cmd('i')
        return self._read_single_reply()

    def get_rtc_time(self):
        self._write_cmd('t')
        return int(self._read_reply_value())

    def set_rtc_time(self, time):
        self._write_cmd('t=%d' % time)
        return self._read_single_reply()

    def get_last_sample(self):
        self._write_cmd('l')
        for l in self._read_reply():
            yield self._parse_sample(l)

    def force_sample(self):
        self._write_cmd('f')
        self._read_reply()

    def set_sample_period(self, period):
        self._write_cmd('T=%d' % period)
        self._read_reply()

    def get_sample_period(self):
        self._write_cmd('T')
        return int(self._read_reply_value())

    def list_sensors(self):
        self._write_cmd('s')
        for l in self._read_reply():
            parts = l.split()
            sensor_id = int(parts[0])
            name = parts[1]
            unit = parts[2]
            yield (sensor_id, name, unit)

    def get_last_sample(self):
        self._write_cmd('l')
        for l in self._read_reply():
            parts = l.split()
            last_sample_time = int(parts[0])
            sensor_id = int(parts[1])
            last_value = float(parts[2])
            yield (last_sample_time, sensor_id, last_value)
