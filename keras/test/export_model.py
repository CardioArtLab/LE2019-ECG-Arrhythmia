import sys, os
import argparse
from keras.models import load_model

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('model_file', help='path to model.hdf5')
    parser.add_argument('--summary', help='show model summary', dest='show_summary', action='store_true')
    args = parser.parse_args()

    model = load_model(args.model_file)
    if args.show_summary:
        model.summary()
        sys.exit()
    json_config = model.to_json()
    dir_path = os.path.dirname(args.model_file)
    model_file = os.path.join(dir_path, 'model.json')
    weight_file = os.path.join(dir_path, 'weights.h5')
    with open(model_file, 'w') as json_file:
        json_file.write(json_config)
    model.save_weights(weight_file)
