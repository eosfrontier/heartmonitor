import serial,evdev,glob,select

class Paddles(object):
    buttons = {}
    def __init__(self):
        self._devlist = {}
        self._rescan = 0
        self._fdlist = {}
        self._serlist = {}
        self._reopen()

    def _reopen(self):
        for jsdev in glob.glob('/dev/input/event*'):
            if not jsdev in self._devlist:
                print "Opening paddle button %s" % jsdev
                js = evdev.InputDevice(jsdev)
                self._devlist[jsdev] = js
                self._fdlist[js.fd] = jsdev
                self.buttons[jsdev] = False
                print "Got paddle button %s" % (self._devlist[jsdev])
        for serdev in glob.glob('/dev/ttyACM*'):
            if not serdev in self._serlist:
                try:
                    print "Opening paddle cmd %s" % serdev
                    self._serlist[serdev] = serial.Serial(serdev)
                except:
                    print "Failed to open!"
                    pass

    def read(self):
        try:
            r, w, x = select.select(self._fdlist, [], [], 0)
            for jsfd in r:
                jsdev = self._fdlist[jsfd]
                for ev in self._devlist[jsdev].read():
                    if ev.type == evdev.ecodes.EV_KEY:
                        if ev.value == 1:
                            self.buttons[jsdev] = True
                        else:
                            self.buttons[jsdev] = False
        except:
            pass
        
    def command(self, line):
        try:
            for serdev in self._serlist.values():
                serdev.write(line+"\r\n")
                serdev.reset_input_buffer()
        except:
            pass
            
