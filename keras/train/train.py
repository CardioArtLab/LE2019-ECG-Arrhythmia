import argparse
import json

import utils
from scipy.io import loadmat
from scipy.stats import zscore
from importlib import import_module

import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import warnings
warnings.filterwarnings('ignore')
import tensorflow as tf
tf.compat.v1.logging.set_verbosity(tf.compat.v1.logging.ERROR)

def train(args, params):
    print ('Load Training Set')
    train = loadmat(params['train'])
    print ('Load Dev Set')
    dev = loadmat(params['dev'])
    train['x'] = zscore(train['x'], axis=1)
    dev['x'] = zscore(dev['x'], axis=1)
    print ('Training Size {} elements'.format(len(train['x'])))
    print ('Dev Size {} elements'.format(len(dev['x'])))

    save_dir = utils.make_save_dir(params['save_dir'], args.experiment)
    
    import keras
    
    stopping = keras.callbacks.EarlyStopping(patience=8)
    reduce_lr = keras.callbacks.ReduceLROnPlateau(factor=0.1, patience=2, min_lr=params['learning_rate'] * 0.01)
    checkpoint = keras.callbacks.ModelCheckpoint(filepath=utils.get_saved_filename(save_dir), save_best_only=True)

    history = model.fit(
        train['x'], train['y'],
        batch_size=params['batch_size'],
        epochs=params['epochs'],
        validation_data=(dev['x'],dev['y']),
        callbacks=[checkpoint, reduce_lr, stopping]
    )

    utils.save(history, save_dir, name='history.bin')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('config_file', help='path to config file (.json)')
    parser.add_argument('--show-compile', '-c', help='see compiled model', dest='show_compile', action='store_true')
    parser.add_argument('--experiment', '-e', help='tag with experiment name', default='default')
    args = parser.parse_args()
    params = json.load(open(args.config_file, 'r'))

    model = import_module(params['model'])
    model = model.build_model(**params)
    if args.show_compile:
        model.summary()        
    else:
        train(args, params)