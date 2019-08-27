import argparse
import fnmatch
import glob
import json
import os
import random

import numpy as np
from scipy import signal
from scipy.io import savemat
from tqdm import tqdm

STEP = 256
RELABEL = {"NSR": "ABN", "SUDDEN_BRADY": "ABN",
           "AVB_TYPE2": "ABN", "AFIB": "ABN", "AFL": "ABN"}

def get_all_records(path, blacklist=set()):
    records = []
    for root, dirnames, filenames in os.walk(path):
        for filename in fnmatch.filter(filenames, '*.ecg'):
            if patient_id(filename) not in blacklist:
                records.append(os.path.join(root, filename))
    return records

def patient_id(record):
    return os.path.basename(record).split("_")[0]

def load_episodes(records=[]):
    episodes = {}
    print('# Make Episode labels')
    for record in tqdm(records):
        file_name = os.path.splitext(record)[0]
        for episode_file in glob.glob(file_name + '*grp*.episodes.json'):
            with open(episode_file, 'r') as fid:
                obj = json.load(fid)
                ep = episodes.get(obj['ECG_ID'], [])
                ep.extend(obj['episodes'])
                episodes[obj['ECG_ID']] = ep
    return episodes

def resample(records):
    lead2_signal = []
    episodes = {}
    signal_index = 0
    #for record in tqdm(records, desc='Resample'):
    labels = set()
    for record in tqdm(records, desc='Resample'):
        with open(record, 'r') as fid:
            signals = np.fromfile(fid, dtype=np.int16)
        signal_250Hz = signal.resample_poly(signals, 5, 4)
        lead2_signal.append(signal_250Hz)
        
        file_name = os.path.splitext(record)[0]
        for episode_file in glob.glob(file_name + '*grp*.episodes.json'):
            with open(episode_file, 'r') as fid:
                obj = json.load(fid)
                ep = episodes.get(signal_index, [])
                _eps = obj['episodes']
                for _ep in _eps:
                    _ep['onset'] *= 5/4
                    _ep['offset'] *= 5/4
                    labels.add(_ep['rhythm_name'])
                    if _ep['rhythm_name'] == 'NOISE':
                        _ep['rhythm_name'] = 'U'
                    else:
                        _ep['rhythm_name'] = 'A'
                ep.extend(_eps)
                episodes[signal_index] = ep
        signal_index += 1
    print(labels)    
    return lead2_signal, episodes

def segmentation(records=[], episodes={}, window=1536, step=1536):
    x_data = []
    y_data = []
    signal_index = 0
    for record in tqdm(records, desc='Segmentation'):
        for start in range(0, len(record)-window, step):
            end = start+window
            y = np.array([0,0,0])
            for ep in episodes.get( signal_index, [] ):
                _start = max(ep['onset'], start)
                _end = min(ep['offset'], end)
                duration = (_end - _start) if _start < _end else 0
                if ep['rhythm_name'] == 'N':
                    y[0] += duration
                elif ep['rhythm_name']  == 'A':
                    y[1] += duration
                elif ep['rhythm_name'] == 'U':
                    y[2] += duration
            x_data.append( record[start:end] )
            y_data.append( y / np.sum(y) )
        signal_index += 1
    return x_data, y_data

if __name__ == "__main__":
    data_dir = "irhythm_2017/"
    records = get_all_records(data_dir)
    ecg, episodes = resample(records)
    x_data, y_data = segmentation(ecg, episodes)
    savemat('test.mat', {'x': x_data, 'y': y_data})