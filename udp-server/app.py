'''
This python script listens on UDP port 3333
for messages from the ESP32 board and prints them
'''
import socketserver
import threading
import queue
import matplotlib.pyplot as plt
import matplotlib.style as mplstyle

# fast and light-weight plotting style
# https://matplotlib.org/tutorials/introductory/usage.html#sphx-glr-tutorials-introductory-usage-py
mplstyle.use(['dark_background', 'fast'])

shutdownEvent = threading.Event()
q = queue.Queue()
ch1_data = []
ch2_data = []

INT16_MAX = 2**15-1
UINT16_MAX = 2**16

class MyUDPHandler(socketserver.BaseRequestHandler):
    '''
    handle the UDP request
    '''
    def handle(self):
        data = self.request[0].strip()
        #socket = self.request[1]
        if (data[0] == 0xCA and data[1] == 0xFE and data[-1] == 0xCD):
            device_id = (data[2])
            packet_id = (data[3])
            ch1_data = []
            ch2_data = []
            for i in range(4, 1204, 4):
                ch1 = (((data[i]) << 8) | (data[i+1]))
                ch2 = (((data[i+2]) << 8) | (data[i+3]))
                if ch1 > INT16_MAX:
                    ch1 -= UINT16_MAX
                if ch2 > INT16_MAX:
                    ch2 -= UINT16_MAX
                ch1_data.append(ch1*4.033/INT16_MAX)
                ch2_data.append(ch2*4.033/INT16_MAX)
            q.put_nowait((ch1_data, ch2_data))
            print(device_id, packet_id)

def reader():
    count = 0
    while True:
        try:
            # if empty queue, the exception will raise
            item = q.get(block=False, timeout=1000)
            ch1, ch2 = item
            if ch1 is not None:
                ch1_data.extend(ch1)
            if ch2 is not None:
                ch2_data.extend(ch2)
            q.task_done()
            # stop condition
            count += 1
            if count > 10:
                shutdownEvent.set()
        except queue.Empty:
            if shutdownEvent.is_set():
                break
            

if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 3333
    with socketserver.ThreadingUDPServer((HOST, PORT), MyUDPHandler) as server:
        # use this in threading
        server_thread = threading.Thread(target=server.serve_forever)
        server_thread.daemon = True
        server_thread.start()
        print('Server Listening: {}'.format(server.server_address))
        # reader in threading
        reader_thread = threading.Thread(target=reader)
        reader_thread.start()
        # normal blocking server
        shutdownEvent.wait(300)
        q.join()
        server.shutdown()
        reader_thread.join()
        # after stop show plot
        plt.plot(ch1_data, '-r', label='ch1', markersize=3)
        plt.plot(ch2_data, '--g', label='ch2', markersize=3)
        plt.legend()
        plt.show()
