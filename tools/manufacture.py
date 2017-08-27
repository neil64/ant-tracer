#!/usr/bin/env python

"""

Tool to batch program ottoq pucks.

"""
import os, re, sys, argparse
from subprocess import Popen
from multiprocessing import Process
from time import sleep
import telnetlib

HEADER = """
-------------------------------------------------------------------------------

Hit [Enter] to manufacture DeviceID: %08x:"""

INSTRUCTIONS = """
Manufacturing for DeviceID (%08x) complete!

Wait for 2 seconds and look at the LED and determine if the programming
was successful.  Here are the possible results:

SUCCESS:
    The LED will be blinking slowly, two beats and a long pause.

FAILURE:
    The LED will be blinking rapidly.

FAILURE:
    The LED will not blink after the install.

If the board was a `FAILURE', please add this device to the bone pile and
make sure it is not powered on (Jumper J1 off).  Grab a new board and restart
this script with this last device id (%08x).

"""


################################################################################

HOST = "localhost"
PORT = 19021

"""
"""
def watch_output():
    while True:
        try:
            tn = telnetlib.Telnet(HOST, PORT)
            s = tn.read_all()
            tr = re.search(r"^SensorTestResult::(.*)$", s, flags=re.MULTILINE)
            di = re.search(r"^\s+DeviceID:\s+(.*)$", s, flags=re.MULTILINE)
            trs = tr.group(1)
            dis = di.group(1)
            return trs, dis
        except:
            pass
        sleep(0.5)

"""
"""
def program_device(id):
    # Update environment.
    env = dict(os.environ)
    if "TARGET" not in env:
        env["TARGET"] = "../puck/main/puck.hex"

    proc = Popen(["./jlprog", "%8x"%id], env=env)
    proc.wait()
    print INSTRUCTIONS % (id, id)

"""
"""
def main():
    #
    #   Setup and parse arguments.
    #
    parser = argparse.ArgumentParser(prog="manufacture")
    parser.add_argument("-d", "--deviceid", required=True,
                        help="DeviceID of first device to program")
    parser.add_argument("-c", "--count", type=int, default=1,
                        help="Number of devices to program")
    args = parser.parse_args()

    start = 0
    try:
        start = int(args.deviceid, 16)
    except ValueError as e:
        print "Error - %s is not a valid (hexadecimal) DeviceID!" % (args.deviceid)

    #
    #   Make some pucks!
    #
    for id in range(start, start+args.count+1):
        raw_input(HEADER % (id))
        jl = Process(target=program_device, args=(id,))
        jl.start()

        tr, devid = watch_output()
        print "TestResult=%s" % tr
        print "DeviceID=%s" % devid

        sleep(0.5)

################################################################################

if __name__ == "__main__":
    main()

################################################################################
