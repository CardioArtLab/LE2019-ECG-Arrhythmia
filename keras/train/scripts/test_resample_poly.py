import wfdb
import matplotlib.pyplot as plt
from scipy.signal import resample_poly
import numpy as np

signal, field = wfdb.rdsamp('data/100', sampto=3600)
y = np.array(resample_poly(signal[:,0], 25, 36)) + 2
plt.plot(y, 'r-')
plt.plot(signal[:,0], 'k')
plt.legend(['new (250Hz, 10s)', 'old (360Hz, 10s)'])
plt.show()
