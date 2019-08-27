import matplotlib.pyplot as plt
import scipy.io as io
import numpy as np
import json
import glob
import os

ecg_file = '../irhythm_2017/ffaa25bd128e6b9661b5cd7f09c4070d_0005.ecg'

with open(ecg_file, 'r') as fid:
    data = np.fromfile(fid, dtype=np.int16)

labels = set()
for json_file in glob.glob(os.path.splitext(ecg_file)[0]+'*'+'.json'):
    with open(json_file, 'r') as fid:
        obj = json.load(fid)
    for ep in obj['episodes']:
        labels.add(ep['rhythm_name'])
print(labels)
plt.plot(data)
plt.show()



