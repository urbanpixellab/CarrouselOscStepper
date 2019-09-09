#!/usr/bin/env python3

from pythonosc import udp_client, osc_message_builder, osc_bundle_builder
from os import system
from socket import AF_INET, SOCK_DGRAM, socket
from time import sleep

debug = True
debug = False

def get_ip_address():
    s = socket(AF_INET, SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    address = s.getsockname()[0]
    s.close()
    return address

if __name__ == "__main__":
    port = 9000
    address = get_ip_address()

    clients = {}
    clients[0] = udp_client.SimpleUDPClient(address, port) # local debug 
    clients[1] = udp_client.SimpleUDPClient('192.168.12.35', port) 
    clients[2] = udp_client.SimpleUDPClient('192.168.12.36', port) 
    clients[3] = udp_client.SimpleUDPClient('192.168.12.37', port) 
    clients[4] = udp_client.SimpleUDPClient('192.168.12.38', port) 
    clients[5] = udp_client.SimpleUDPClient('192.168.12.39', port) 
    clients[6] = udp_client.SimpleUDPClient('192.168.12.40', port) 
    
    while True:
#        system('clear')
        print('''OSC TEST CLIENT

Enter a command consisting of:
    a command [0-9]
    a global! motor ID [0-9][0-9]
    an optional percentage [0-9][0-9][0-9]
        
For example to stop all motors:
    000
    
For example to initialize all motors:
    100
    
For example to move global motor 13 to maximum of its range:
    313

For example to move global motor 9 (local motor A on controller 3) to 37% of its range:
    409037

Commands:
    0 stop motors*
    1 initialize motors*
    2 go to motor minimum*
    3 go to motor maximum*
    4 go to % of motor range**
    5 oscillate linearly between current position and % of motor range**
    6 for one hour go to alternating extrema adn wait 5 seconds
    9 quit this client application

*   Command always works directly, also without initialization.
**  Command only works only after initialization has been completed and
    do not start when command 1, 2 or 3 is running!
    
    Stopping command 1,2 or 3 requires new initialization!

Global motor IDs:
    00 all motors
    01 motor with ID 01 (local motor 1 (A) on controller 1)
    02 motor with ID 02 (local motor 2 (B) on controller 1)
    ...
    20 motor with ID 20 (local motor 4 (D) on controller 5)
''')
        line = input('Enter command: ')
        c = int(line[0])

        if c == 9:
            print('INFO: quit')
            exit(0)

        if len(line) < 3:
            print('ERROR: Incomplete command {}'.format(line))
            input('Press enter to continue:')
            continue
        motor = int(line[1:3])
        m = ((motor - 1) % 4) + 1
        i = []
        i.append(((motor - 1) // 4) + 1)
        if motor == 0:
            i = []
            i.append(1)
            i.append(2)
            i.append(3)
            i.append(4)
            i.append(5)
            i.append(6)
            m = 0

        a = 0
        if len(line) == 6:
            a = int(line[3:])
        
        value = m * 1000 + a
        l = ''
        if m != 0:
            l = chr(ord('a') + m - 1).upper()
        
        if c == 0:
#            value += c * 10000
            print('INFO: stop c={} motor={} (i={} m={} l={}) value={:05}'.format(c, motor, i, m, l, value))
        elif c == 1:
            value += c * 10000
            print('INFO: initialize c={} motor={} (i={} m={} l={}) value={:05}'.format(c, motor, i, m, l, value))
        elif c == 2:
            value += c * 10000
            print('INFO: minimum c={} motor={} (i={} m={} l={}) value={}'.format(c, motor, i, m, l, value))
        elif c == 3:
            value += c * 10000
            print('INFO: maximum c={} motor={} (i={} m={} l={}) value={}'.format(c, motor, i, m, l, value))
        elif c == 4:
            value += c * 10000
            print('INFO: go to c={} motor={} (i={} m={} l={}) a={} value={}'.format(c, motor, i, m, l, a, value))
        elif c == 5:
            value += c * 10000
            print('INFO: oscillate c={} motor={} (i={} m={} l={}) a={} value={}'.format(c, motor, i, m, l, a, value))
        elif c == 6:
            value += 2 * 10000 # min
            print('INFO: oscillate c={} motor={} (i={} m={} l={}) a={} value={}'.format(c, motor, i, m, l, a, value))
        else:
            print('ERROR: Unknown command {}'.format(line))
            input('Press enter to continue:')
            continue
 
        if c == 6:
                sl = 5
                for x in range(60 * sl):
                        print('{}/{}'.format(x+1, 60 * sl))
                        if value // 10000 == 3: # max
                                value -= 10000 # min
                        else: # min
                                value += 10000 # max
                        print(value)
                        bundle = osc_bundle_builder.OscBundleBuilder(osc_bundle_builder.IMMEDIATELY)
                        message = osc_message_builder.OscMessageBuilder(address='/input')
                        message.add_arg(value)
                        bundle.add_content(message.build())
                        if debug:
                            clients[0].send(bundle.build())
                        else:
                            for id in i:
                                clients[id].send(bundle.build())
                        sleep(sl)

        else:
                bundle = osc_bundle_builder.OscBundleBuilder(osc_bundle_builder.IMMEDIATELY)
                message = osc_message_builder.OscMessageBuilder(address='/input')
                message.add_arg(value)
                bundle.add_content(message.build())
                if debug:
                    clients[0].send(bundle.build())
                else:
                    for id in i:
                        clients[id].send(bundle.build())
        
                input('Press enter to continue:')
