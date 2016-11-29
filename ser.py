#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# pip install pySerial

"""
Created on  9 Feburary 2016
@author: Ole Dam Møller
"""

import serial
import re
import time
import httplib, urllib
import sys, traceback
import glob
import termios

debug = 0
uploadinterval = 20
tskey = 'yourkey' #pHerminator channel

# mes: Humidity val:  26.00
# mes: Temperature val:  27.00
# mes: GasLeakage val:   1.19

mapping = {
		'CO'      :'field1',
		'H'       :'field2',
		'Alcohol' :'field3',
		'NH3'     :'field4',
		'PH'      :'field5',
                'MQ7RAW'  :'log',
                'MQ3RAW'  :'log',
                'MQ8RAW'  :'log',
                'MQ135RAW':'log',
                'PHRAW'   :'log',
                'RELHUM'  :'field6',
                'TEMP'    :'field7',
                'ATEMP'   :'field8'
            }



def matchline (line, measures):
	regex = re.compile(r'^#([^ ]+) +([^ ]+) +([0-9\.]+)', flags=re.IGNORECASE)
	m = regex.match(line)
	if m:
		mes = m.group(1)
		unit = m.group(2)
		val = float(m.group(3))
                if debug:
		    print "mes: %-6s val: %6.2f" % ( mes, val )
		if mes not in measures:
		    measures[mes] = list()
		    measures[mes].append( val )
	elif debug:
		print '          DEBUG:', line
        return measures

def consolidate (measures):
	values = {}
	for key in measures.keys():
            if debug:
	        print "UP mes: %-6s num: %3i avg: %6.2f" % ( key, len(measures[key]), sum(measures[key])/len(measures[key]) )
	    values[key] = sum(measures[key])/len(measures[key])
	return values


def serial_ports(used=[]):
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        if port not in used:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
            except (OSError, serial.SerialException):
                pass

    return result



def opendev(device):


	# Disable reset after hangup
	with open(device) as f:
    		attrs = termios.tcgetattr(f)
    		attrs[2] = attrs[2] & ~termios.HUPCL
    		termios.tcsetattr(f, termios.TCSAFLUSH, attrs)

	ser = serial.Serial(device, 9600, timeout=0)
        return ser

def collect(ser):
        """Collects sensor readings on serial inderface aka usb-port

        :param device: serialdevice to listen to

	Opens the serial port and listens for observations in the form:

        ``#MEASSURE UNIT VALUE``

        Where:

        ``MESSSURE`` is a unique string describing the sensortype

        ``UNIT``     is the unit of the meassurement

        ``VALUE``    is the numeric value



        .. todo:: more errorhandling
        """
        try:
    	    line = readLine(ser)
        except serial.SerialException:
            return -1

        return line

def readLine(ser):
    str = ""
    try:
        if not ser.inWaiting():
	    return str

        while 1:
            ch = ser.read()
            if(ch == '\r' or ch == '\n'):
                break
            #if(ch == '' ):
            str += ch
    except:
        str = ""

    return str



def upload(values):
        """Sends data to our thingspeakchannel

        :param values: a dictionary with data to be uploaded

        Tests every key, to see if it maps to a field in the mapping dictionary
        and builds up a ``POST`` string to send to https://thingspeak.com/channels/84694


        .. todo:: add channel and mapping to parameters
        """

	#our key for thingspeak
	params = { 'key': tskey }
	for key in values.keys():
		if key in mapping:
			#add mapped fields
                        if not mapping[key] == "log":
			    params[mapping[key]] = values[key]
		else:
			print "Error %s is unknown i mapping" % key

	headers = {"Content-type": "application/x-www-form-urlencoded","Accept": "text/plain"}
	conn = httplib.HTTPConnection("api.thingspeak.com:80")


	try:
		conn.request("POST", "/update", urllib.urlencode(params), headers)
		response = conn.getresponse()
		data = response.read()
		conn.close()
	except:
		print "connection failed"
		print time.strftime("%a, %d %b %Y %H:%M:%S", time.localtime())
                try:
		    print response.status, response.reason
                except UnboundLocalError:
                    pass

if __name__ == "__main__":
	usage = """
Usage: ser [-d] serialdevice

    Reads serial device line by line and accumulte readings
    -d enables debug

    Example:
      ser.py -d
"""

	if sys.argv[1] == '-d':
		debug = 1

        lasterror  = ''
        errorcount = 0
        errorstate = False

        try:

            measures = dict()
            arduinos = dict()
            nextup = time.time()+uploadinterval

            while True:
                deadports = dict()
                usbports = serial_ports(arduinos)
                for p in usbports:
                    s = opendev(p)
                    arduinos[p] = s
                for a in arduinos:
	            l = collect(arduinos[a])
                    if l == -1:
                        deadports[a] = True
                    elif l:
                        measures = matchline(l, measures)
                for a in deadports:
                    arduinos.pop(a,None)
                errorstate = False
                errorcount = 0

	        if time.time() > nextup:
                    vals = consolidate(measures)
                    upload(vals)
	            measures = {}
                    nextup = time.time()+uploadinterval

        except KeyboardInterrupt:
            pass
        except:
            error = sys.exc_info()[0]
            errorcount += 1
            if error != lasterror:
                print "Exception in collect code:"
                print '-'*60
                traceback.print_exc(file=sys.stdout)
                print '-'*60
            lasterror = error
            errorstate = True
            if errorcount > 100:
                print "Permanent error state %s" % error
                sys.exit(1)


