import os
import pickle
import time
import random

def load(dirname, name='preproc.bin'):
    preproc_f = os.path.join(dirname, name)
    with open(preproc_f, 'rb') as fid:
        preproc = pickle.load(fid)
    return preproc

def save(preproc, dirname, name='preproc.bin'):
    preproc_f = os.path.join(dirname, name)
    with open(preproc_f, 'wb') as fid:
        pickle.dump(preproc, fid)

def make_save_dir(dirname, experiment_name):
    start_time = str(int(time.time())) + '-' + str(random.randrange(1000))
    save_dir = os.path.join(dirname, experiment_name, start_time)
    if not os.path.exists(save_dir):
        os.makedirs(save_dir)
    return save_dir

def get_saved_filename(save_dir):
    return os.path.join(save_dir, "{val_loss:.3f}-{val_acc:.3f}-{epoch:03d}.hdf5")