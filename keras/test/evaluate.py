import os, sys
import argparse
import keras
from scipy.io import loadmat, savemat
from scipy.stats import zscore
import numpy as np
import matplotlib.pyplot as plt
from sklearn.metrics import precision_recall_fscore_support as score
from sklearn.metrics import confusion_matrix

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('model_path', help='path to model.hdf5')
    parser.add_argument('--summary', help='show model summary', dest='show_summary', action='store_true')
    parser.add_argument('--test_file', '-t', help='path to test.mat', default='test.mat')
    args = parser.parse_args()

    model_file = os.path.join(args.model_path, 'model.json')
    weight_file = os.path.join(args.model_path, 'weights.h5')
    result_file = os.path.join(args.model_path, 'result.mat')
    with open(model_file) as json_file:
        json_config = json_file.read()
    model = keras.models.model_from_json(json_config)
    model.load_weights(weight_file)
    
    if args.show_summary:
        model.summary()
        sys.exit()
    if not os.path.exists(args.test_file):
        print('Not found: {:}'.format(args.test_file))
        sys.exit()
        
    data = loadmat(args.test_file)
    x_data = zscore(data['x'], axis=1)
    y_data = data['y']
    if len(data['y'].shape) > 2:
        y_data = y_data[:,0]
    pred_y = model.predict(x_data, batch_size=32)
    if len(pred_y.shape) > 2:
        pred_y = pred_y[:,0]
    print('Loss')
    loss = -1 * np.multiply(y_data, np.log(pred_y + 1e-18))
    print('Max: {:}, Mean: {:}, Min: {:}'.format(np.max(loss),np.mean(loss), np.min(loss)))
    #acc = np.equal(np.argmax(y_data, axis=-1), np.argmax(pred_y, axis=-1))
    #print('Accuracy: {:}'.format(np.sum(acc)/np.size(acc)))
    graded_true = np.argmax(y_data, axis=-1)
    graded_predict = np.argmax(pred_y, axis=-1)
    
    precision, recall, fscore, support = score(graded_true, graded_predict)
    confusion_mat = confusion_matrix(graded_true, graded_predict)
    
    print('Confusion Matrix')
    print(confusion_mat)
    
    print('Precision')
    print(precision)
    
    print('Recall')
    print(recall)
    
    print('f1 score')
    print(fscore)
    
    print('Support')
    print(support)
    
    savemat(result_file, {
            'loss': loss,
            'y_true': y_data,
            'y_pred': pred_y,
            'confusion_matrix': confusion_mat,
            'precision': precision,
            'recall': recall,
            'fscore': fscore,
            'support': support
    })

