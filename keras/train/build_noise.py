from glob import glob
import wfdb
import numpy as np
from scipy import signal
import scipy.io as sio
from tqdm import tqdm
import os

def resample_all_noise(records):
    
    lead2_signal = []
    lead2_annotation = []

    for i in tqdm(records, desc='Resample'):    
        _,fields = wfdb.rdsamp(i, sampto=1)
        for ch in range(len(fields['sig_name'])):
            signals, _ = wfdb.rdsamp(i, channels = [ch])
            signal_250Hz = signal.resample_poly(signals, 25, 36)
            lead2_signal.append(signal_250Hz)
            if os.path.exists(i+'.atr'):
                ann = wfdb.rdann(i, 'atr')
                sample_250Hz = ann.sample*25/36
                annotation = list(zip(sample_250Hz,ann.symbol))
            else:
                annotation = [(x, 'U') for x in range(0,len(signal_250Hz),50)]
            lead2_annotation.append(annotation)
        
    return lead2_signal, lead2_annotation

def resample(records):
    lead2_signal = []
    lead2_annotation = []
    
    for i in tqdm(records, desc='Resample with noise'):
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
#%%
def noise_annotation(annotation, secs, sampling_rate=250):
    new_annotation = []
    for ann in tqdm(annotation, desc='Annotation noise'):
        new_ann = []
        for value in ann:
            new_value = value
            for (start, end) in secs:
                if value[0] >= start*sampling_rate and value[0] <= end*sampling_rate:
                    new_value = (value[0],'U')
            new_ann.append(new_value)
        new_annotation.append(new_ann)
    return new_annotation

#lead2_signal, lead2_annotation = resample(records)
#%%
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
def build_noise():
    labels = [
        ['N','·'],
        ['L','R','B','A','a','J','S','V','r','F','e','j','n','E','/','f','!','(AB','(AFIB','(AFL','(B','(BII','(IVR','(PREX','(SBR','(SVTA','(T','(VFL','(VT'],
        ['Q','?','qq','U','M','MISSB','P','PSE','T','TS']
    ]    
    
    x_data = []
    lead2_signal, lead2_annotation = resample_all_noise(['mitdb-noise/em', 'mitdb-noise/ma', 'mitdb-noise/bw'])
    _x, _ann = segmentation(lead2_signal, lead2_annotation, window=1536, step=200)
    y_data = new_annotation(_ann, labels)
    x_data.extend(_x)
    
    
    lead2_signal, lead2_annotation = resample([
        'mitdb-noise/118e_6', 'mitdb-noise/118e00', 'mitdb-noise/118e06', 'mitdb-noise/118e12',
        'mitdb-noise/119e_6', 'mitdb-noise/119e00', 'mitdb-noise/119e06', 'mitdb-noise/119e12'])
    _ann = noise_annotation(lead2_annotation, [(x,x+2*60) for x in range(5*60,31*60,4*60)], sampling_rate=250)
    _x, _ann = segmentation(lead2_signal, _ann, window=1536, step=200)
    _y = new_annotation(_ann, labels)
    x_data.extend(_x)
    y_data.extend(_y)  

    return x_data, y_data