import socket
from time import sleep, time
import threading
from random import randrange

SERVER_ADDR = (input("ip: "), int(input("port: ")))

PACKAGE_SIZE = int(input("package size: "))

TIME_OUT = float(input("time out: "))

file_size = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(( "0.0.0.0", 0 ))
sock.settimeout(TIME_OUT)




bytes_recved = 0
packets_sent = 0
packets_recved = 0
### tests = list of tuples ( func, name, dont_ask_var ) if func returns true than the recved packege is wrong
def recv_data( reqtype: bytes, reqcont: bytes, tests=[] ) -> bytes:
    global bytes_recved, packets_sent, packets_recved
    dont_ask = False
    out = b""
    reqid = 0
    while 1:
        try:
            if not dont_ask:
                reqid = randrange(1000000)
                sock.sendto( reqtype[:4] + reqid.to_bytes(4, "big") + reqcont, SERVER_ADDR )
                packets_sent += 1

            data, addr = sock.recvfrom( 4096 )

            if addr != SERVER_ADDR:
                print( f"recved from wrong addr SERVER_ADDR != {addr}" )
                dont_ask = True
                continue

            if int.from_bytes(data[:4],"big") != reqid:
                print( "wrong reqid", reqid, int.from_bytes(data[:4],"big") )
                dont_ask = True
                continue

            out = data[4:]


            for test in tests:
                if test[0](out):
                    print(test[1], "data:", out)
                    dont_ask = test[2]


            packets_recved += 1
            bytes_recved += len(out)

            break

        except TimeoutError:
            print(".", end="")
        except Exception as e:
            print(e)
        dont_ask = False

    return out

    



print( "setting package size" )

data = recv_data(b"PKGS", PACKAGE_SIZE.to_bytes(2, "big"))
PACKAGE_SIZE = int.from_bytes(data, "big")
print(f"package size is {PACKAGE_SIZE}")


print("getting file size")
data = recv_data(b"GFS_", b"")
file_size = int.from_bytes( data[:8], "big" )
print( f"filesize is {file_size}" )


package_count = file_size//PACKAGE_SIZE + (file_size%PACKAGE_SIZE!=0)
file = open("out", "wb")
sock.settimeout(0.1)








monitor_running = True
time_point = time()
bytes_recved = 0
bytes_all_recved = 0
packets_sent = 0
packets_recved = 0
def monitor_target():
    global time_point, bytes_recved, bytes_all_recved, packets_sent, packets_recved
    while monitor_running:
        tn = time()
        bytes_all_recved += bytes_recved
        if packets_sent == 0:
            packets_sent = 1
        print( f"{round(100*bytes_all_recved/file_size, 3)}%\t   lost ratio: {round(100*packets_recved/packets_sent, 3)}%\t   downspeed: {round(bytes_recved*8/1000000/(tn-time_point), 3)} Mb/s" )
        bytes_recved = 0
        time_point = tn
        packets_sent = 0
        packets_recved = 0
        sleep(1)

monitor_thread = threading.Thread(target=monitor_target)
monitor_thread.start()






for i in range(package_count):
    def package_idx_test(data:bytes):
        global i
        return i != int.from_bytes(data[:4], "big")

    data = recv_data(b"GPBI", i.to_bytes(4, "big"), [(package_idx_test, "package idx", True)])

    packet_idx = int.from_bytes( data[:4], "big" )

    file.write(data[4:])

for i in range(100):
    sock.sendto( b"FINH", SERVER_ADDR )

monitor_running = False
monitor_thread.join()
file.close()