from glob import glob
import wfdb
import numpy as np
from scipy import signal
import scipy.io as sio
from tqdm import tqdm
import random
import build_noise

def get_records():
    # There are 3 files for each record
    # *.atr is one of them
    direc = glob('mitdb\*.atr')
    # Get rid of the extension
    direc = [path[:-4] for path in direc] #-.atr
    direc.sort()
    return direc

#records = get_records()

def resample(records):
    
    lead2_signal = []
    lead2_annotation = []

    for i in tqdm(records, desc='Resample'):    
        _,fields = wfdb.rdsamp(i, sampto=1)
        if 'MLII' in fields['sig_name']:
            ch = fields['sig_name'].index('MLII')
            signals, _ = wfdb.rdsamp(i, channels = [ch])
            signal_250Hz = signal.resample_poly(signals, 25, 36)
            lead2_signal.append(signal_250Hz)
            ann = wfdb.rdann(i, 'atr')
            sample_250Hz = ann.sample*25/36
            annotation = list(zip(sample_250Hz,ann.symbol))
            lead2_annotation.append(annotation)
        
    return lead2_signal, lead2_annotation

#lead2_signal, lead2_annotation = resample(records)

def segmentation(lead2_signal, lead2_annotation, window=1500, step=1500):
          
    x_data = []
    annotation = []
    for i,_ in enumerate(tqdm(lead2_signal, desc='Segmentation')):
        for start in range(0,len(lead2_signal[i])-window,step):   
            x_data.append(lead2_signal[i][start:start+window])
            ann = [y for x,y in lead2_annotation[i] if (x>=start and x<start+window)]
            annotation.append(ann)
    return x_data, annotation

#x_data, annotation = segmentation(lead2_signal, lead2_annotation)

def new_annotation(annotation, labels):
    y_data = []
    for i in tqdm(annotation, desc='Annotation'):
        label_no = []
        for label in labels:
            label_no.append(np.sum(np.in1d(i, label)))
        label_no = np.array(label_no)/np.sum(label_no)
        y_data.append(label_no)
    return y_data

#labels = [['N','·'], ['L','R','B','A','a','J','S','V','r','F','e','j','n','E','/','f','!','(AB','(AFIB','(AFL','(B','(BII','(IVR','(PREX','(SBR','(SVTA','(T','(VFL','(VT'], ['Q','?','qq','U','M','MISSB','P','PSE','T','TS']]         
#y_data = new_annotation(annotation, labels)
       
def build_dataset(x_data, y_data, dev_frac=0.1, test_frac=0.1):
    index_arr = list(range(len(x_data)))
    random.shuffle(index_arr)
    
    train_end = int(( 1 - (dev_frac + test_frac)) *len(x_data))
    train_index = index_arr[:train_end]
    dev_end = int((1-test_frac) * len(x_data))
    dev_index = index_arr[train_end:dev_end]
    test_index = index_arr[dev_end:]
    
    x_data = np.array(x_data)
    y_data = np.array(y_data)
    
    train_x = x_data[(train_index)]
    train_y = y_data[(train_index)]
    
    dev_x = x_data[(dev_index)]
    dev_y = y_data[(dev_index)]
    
    test_x = x_data[(test_index)]
    test_y = y_data[(test_index)]
    
    sio.savemat('train.mat', {'x': train_x, 'y': train_y})
    sio.savemat('dev.mat', {'x': dev_x, 'y': dev_y})
    sio.savemat('test.mat', {'x': test_x, 'y':test_y})

def build_data():
    records = get_records()
    lead2_signal, lead2_annotation = resample(records)
    x_data, annotation = segmentation(lead2_signal, lead2_annotation, window=1536, step=1000)
    labels = [
        ['N','·'],
        ['L','R','B','A','a','J','S','V','r','F','e','j','n','E','/','f','!','(AB','(AFIB','(AFL','(B','(BII','(IVR','(PREX','(SBR','(SVTA','(T','(VFL','(VT'],
        ['Q','?','qq','U','M','MISSB','P','PSE','T','TS']
    ]         
    y_data = new_annotation(annotation, labels)
    return x_data, y_data

if __name__ == "__main__":
    x_data, y_data = build_data()
    _x, _y = build_noise.build_noise()
    x_data.extend(_x)
    y_data.extend(_y)
    build_dataset(x_data, y_data)