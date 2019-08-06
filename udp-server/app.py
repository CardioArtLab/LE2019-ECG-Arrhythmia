'''
This python script listens on UDP port 3333
for messages from the ESP32 board and prints them
'''
import socketserver
import threading
import queue
from statistics import median
from math import sqrt
import matplotlib.pyplot as plt
import matplotlib.style as mplstyle
from scipy.signal import savgol_filter

# fast and light-weight plotting style
# https://matplotlib.org/tutorials/introductory/usage.html#sphx-glr-tutorials-introductory-usage-py
mplstyle.use(['dark_background', 'fast'])

shutdownEvent = threading.Event()
q = queue.Queue()
ch1_data = []
ch2_data = []

INT16_MAX = 2**15-1
UINT16_MAX = 2**16
MAX_RECORD = 10
HAMPEL_WINDOW = 5
SG_FILTER_WINDOW = 11
SG_COFF = [-42,-21,-2,15,30,43,54,63,70,75,78,79,78,75,70,63,54,43,30,15,-2,-21,-42]
SG_H = 805
#SG_COFF = [-253,-138,-33,62,147,222,287,343,387,422,447,462,467,462,444,422,387,343,287,222,147,62,-33,-138,-253]
#SG_H = 5175

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
                ch1_data.append(ch1)
                ch2_data.append(ch2)
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
            if count > MAX_RECORD:
                shutdownEvent.set()
        except queue.Empty:
            if shutdownEvent.is_set():
                break
            
def hampel(data = [], window=HAMPEL_WINDOW, k=1.5):
    length_data = len(data)
    for i in range(length_data):
            # Hampel filter: remove outliner
            if i-window >= 0:
                start_i = i-window
            else:
                start_i = 0
            seq = data[start_i:i+window]
            if len(seq) > 0:
                m = median(seq)
                sd = 0
                for x in seq:
                    sd += (x - m)**2
                sd = sqrt(sd / len(seq))
                if abs(data[i] - m) > k*sd:
                    data[i] = m
    return data

def smooth(data=[]):
    filtered = []
    length_data = len(data)
    for i in range(length_data):
        # Savitzky-Golay filter
        if i-SG_FILTER_WINDOW >= 0:
            start_i = i-SG_FILTER_WINDOW
        else:
            start_i = 0
        seq = data[start_i:i+SG_FILTER_WINDOW]
        _sum = 0
        for ii in range(len(seq)):
            _sum += seq[ii] * SG_COFF[ii]
        filtered.append(_sum/SG_H)
    return filtered

def kalman(data=[]):
    Q = 1e-5 # process variance
    # allocate space for arrays
    xhat=[data[0]]
    P=[1.0]
    xhatminus=[0]
    Pminus=[0]
    K=[0]
    R = 0.0001 # estimate of measurement variance, change to see effect

    for k in range(1,len(data)):
        # time update
        xhatminus.append(xhat[k-1])
        Pminus.append(P[k-1]+Q)

        # measurement update
        K.append( Pminus[k]/( Pminus[k]+R ) )
        xhat.append( xhatminus[k]+K[k]*(data[k]-xhatminus[k]) )
        P.append( (1-K[k])*Pminus[k] )
    return xhat

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
        
        ch1 = kalman(savgol_filter(hampel(ch1_data, HAMPEL_WINDOW, 1.5), 41, 1))
        ch2 = kalman(savgol_filter(hampel(ch2_data, HAMPEL_WINDOW, 1.5), 41, 1))
     
        ch3 = []
        aVR = []
        aVL = []
        aVF = []
        for i in range(len(ch1)):
            ch3.append(ch2[i] - ch1[i])
            aVR.append(-0.5*(ch1[i]+ch2[i]))
            aVL.append(ch1[i]-0.5*ch2[i])
            aVF.append(ch2[i]-0.5*ch1[i])        
        
        plt.plot(ch1, 'g', label='lead I', markersize=1, linewidth=0.5)
        plt.plot(ch2, 'r', label='lead II', markersize=1, linewidth=0.5)
        plt.plot(ch3, 'y', label='lead III', markersize=1, linewidth=0.5)
        plt.plot(aVR, 'b', label='aVR', markersize=1, linewidth=0.5)
        plt.plot(aVL, 'w', label='aVL', markersize=1, linewidth=0.5)
        plt.plot(aVF, 'c', label='aVF', markersize=1, linewidth=0.5)
        plt.legend()
        plt.show()
