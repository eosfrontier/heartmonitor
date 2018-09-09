import MAX30102
import time

mx = MAX30102()

while True:
    mx.update()
    print "Red: %18.4f IR: %18.4f\n" % (mx.red, mx.ir)
    time.sleep(0.02)
