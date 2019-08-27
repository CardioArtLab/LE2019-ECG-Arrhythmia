import os, sys
import argparse
import numpy as np
import matplotlib.pyplot as plt
import utils

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path', help='path to model.hdf5')
    args = parser.parse_args()

    data = utils.load(args.path)
    for key in data.history.keys():
        if key in ['val_acc', 'val_loss', 'acc', 'loss']:
            plt.plot(data.history[key], 'x-', label=key)
    plt.legend()
    plt.show()

