from glob import glob
from evdev import InputDevice
from select import select

class Paddles(object):
    buttons = {}
    def __init__(self):
        self._devlist = {}
        self._rescan = 0
        self._fdlist = {}

    def _reopen(self):
        for jsdev in glob('/dev/input/js*'):
            if not jsdev in _devlist:
                try:
                    _devlist[jsdev] = InputDevice(jsdev)
                    _fdlist[js.fd] = jsdev
                    buttons[jsdev] = False
                except:
                    pass

    def read(self):
        try:
            r, w, x = select(_devlist.values(), [], [], 0)
            for jsfd in r:
                jsdev = _fdlist[jsfd]
                for ev in _devlist[jsdev].read():
                    if ev.type == ecodes.EV_KEY:
                        if ev.value == 1:
                            buttons[jsdev] = True
                        else:
                            buttons[jsdev] = False
        except:
            pass
