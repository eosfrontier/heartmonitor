import nfc,ctypes

class NFCReader(object):
    MC_AUTH_A = 0x60
    MC_AUTH_B = 0x61
    MC_READ = 0x30
    MC_WRITE = 0xA0
    card_timeout = 10

    def __init__(self):
        self.__context = None
        self.__device = None
        
        self._card_present = False
        self._card_last_seen = None
        self._card_uid = None
        
        mods = [(nfc.NMT_ISO14443A, nfc.NBR_106)]
        self.__modulations = (nfc.nfc_modulation * len(mods))()
        for i in range(len(mods)):
            self.__modulations[i].nmt = mods[i][0]
            self.__modulations[i].nbr = mods[i][1]

    def run(self):
        self.__context = ctypes.pointer(nfc.nfc_context())
        nfc.nfc_init(ctypes.byref(self.__context))
        conn_strings = (nfc.nfc_connstring * 10)()
        devices_found = nfc.nfc_list_devices(self.__context, conn_strings, 10)
        if devices_found >= 1:
            self.__device = nfc.nfc_open(self.__context, conn_strings[0])
            _ = nfc.nfc_initiator_init(self.__device)

    def poll(self):
        nt = nfc.nfc_target()
        res = nfc.nfc_initiator_poll_target(self.__device, self.__modulations, len(self.__modulations), 1, 1, ctypes.byref(nt))
        if res < 0:
            return None
        elif res == 0:
            return None
        uid = None
        if nt.nti.nai.szUidLen == 4:
            uid = "".join([format(nt.nti.nai.abtUid[i],'02x') for i in range(4)])
        return uid

    def __del__(self):
        nfc.nfc_close(self.__device)
        nfc.nfc_exit(self.__context)

