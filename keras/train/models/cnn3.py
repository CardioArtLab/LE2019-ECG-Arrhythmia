from keras import backend as K
from keras.models import Model
from keras.layers import Input, Activation, Dense, Conv1D, BatchNormalization, MaxPool1D
from keras.layers.core import Lambda
from keras.layers.wrappers import TimeDistributed
from keras.optimizers import Adam

'''
Build model
'''
def build_model(**params):
    inputs = Input(shape=(params['input_size'], 1), dtype='float32', name='inputs')
    #layer = Reshape((1,params['input_size'][0]))(inputs)
    layer = Conv1D(256, 5, strides=2, padding='same')(inputs)
    layer = MaxPool1D()(layer)
    layer = BatchNormalization()(layer)
    layer = Activation('relu')(layer)    
    layer = Conv1D(128, 3, strides=2, padding='same')(layer)
    layer = MaxPool1D()(layer)
    layer = BatchNormalization()(layer)
    layer = Activation('relu')(layer)    
    layer = Conv1D(64, 3, strides=2, padding='same')(layer)
    layer = MaxPool1D()(layer)
    layer = BatchNormalization()(layer)
    layer = Activation('relu')(layer)    
    layer = Conv1D(32, 22, strides=2)(layer)
    layer = MaxPool1D()(layer)
    layer = Dense(params['output_size'])(layer)
    output = Activation('softmax')(layer)
    # customize optimizer
    optimizer = Adam(lr=params['learning_rate'], clipnorm=params.get('clipnorm', 1))
    model = Model(inputs=[inputs], outputs=[output])
    model.compile(loss='categorical_crossentropy', optimizer=optimizer, metrics=['accuracy'])
    return model