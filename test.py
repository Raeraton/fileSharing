import socket
from time import sleep, time
import threading
from random import randrange

SERVER_ADDR = (input("ip: "), int(input("port: ")))

PACKAGE_SIZE = int(input("package size: "))
file_size = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(( "0.0.0.0", 0 ))

print( "setting package size" )
sock.settimeout(1)
while 1:
    try:
        reqid = randrange(1000000)
        sock.sendto( b"PKGS" + reqid.to_bytes(4,"big") + PACKAGE_SIZE.to_bytes(2,"big"), SERVER_ADDR )
        data, addr = sock.recvfrom(1024)
        if addr != SERVER_ADDR: 
            print( "wrong address", SERVER_ADDR, addr )
            continue

        if int.from_bytes(data[:4],"big") != reqid:
            print( "wrong reqid", reqid, int.from_bytes(data[:4],"big") )
            continue

        PACKAGE_SIZE = int.from_bytes( data[4:6], "big" )

        break

    except TimeoutError:
        print(".", end="")
    except Exception as e:
        print(e)

print(f"package size is {PACKAGE_SIZE}")


print("getting file size")
while 1:
    try:
        reqid = randrange(1000000)
        sock.sendto( b"GFS_" + reqid.to_bytes(4,"big"), SERVER_ADDR )
        data, addr = sock.recvfrom(1024)
        if addr != SERVER_ADDR: 
            print( "wrong address", SERVER_ADDR, addr )
            continue
        if int.from_bytes(data[:4],"big") != reqid: 
            print( "wrong reqid", reqid, int.from_bytes(data[:4],"big") )
            continue

        file_size = int.from_bytes( data[4:12], "big" )

        break

    except TimeoutError:
        
        print(".", end="")
    except Exception as e:
        print(e)

print( f"filesize is {file_size}" )


package_count = file_size//PACKAGE_SIZE + (file_size%PACKAGE_SIZE!=0)
file = open("out", "wb")
sock.settimeout(0.1)








monitor_running = True
time_point = time()
bytes_recved = 0
bytes_all_recved = 0
def monitor_target():
    global time_point, bytes_recved, bytes_all_recved
    while monitor_running:
        tn = time()
        bytes_all_recved += bytes_recved
        print( f"{100*bytes_all_recved//file_size}\t\t\t   downspeed: {bytes_recved*8/1000000/(tn-time_point)} Mb/s" )
        bytes_recved = 0
        time_point = tn
        sleep(1)

monitor_thread = threading.Thread(target=monitor_target)
monitor_thread.start()







for i in range(package_count):
    while 1:
        try:

            reqid = randrange(1000000)
            sock.sendto( b"GPBI" + reqid.to_bytes(4,"big") + i.to_bytes(4,"big"), SERVER_ADDR )
            data, addr = sock.recvfrom(4096)

            if addr != SERVER_ADDR:
                print("resp not from the server")
                continue
            if int.from_bytes(data[:4],"big") != reqid:
                print(f"wrong req id. expected: {reqid}  got:{int.from_bytes(data[:4],"big")}")
                continue

            package_idx = int.from_bytes(data[4:8])
            if package_idx != i:
                print( f"wrong packet id. expected: {i}  got: {package_idx}" )
                continue

            data = data[8:]

            bytes_recved += len(data)

            file.write(data)

            break
            
        except TimeoutError:
            print(".", end="")
        except Exception as e:
            print(e)


for i in range(100):
    sock.sendto( b"FINH", SERVER_ADDR )

monitor_running = False
monitor_thread.join()
file.close()