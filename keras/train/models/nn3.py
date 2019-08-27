from keras import backend as K
from keras.models import Model
from keras.layers import Input, Activation, Dense, Reshape
from keras.layers.core import Lambda
from keras.layers.wrappers import TimeDistributed
from keras.optimizers import Adam

'''
Build model
'''
def build_model(**params):
    inputs = Input(shape=(params['input_size'],), batch_shape=(None, params['input_size'], 1), dtype='float32', name='inputs')
    layer = Reshape((1, params['input_size']))(inputs)
    layer = Dense(256, activation='relu')(layer)
    layer = Dense(256, activation='relu')(layer)
    layer = Dense(256, activation='relu')(layer)
    layer = Dense(params['output_size'])(layer)
    output = Activation('softmax')(layer)
    # customize optimizer
    optimizer = Adam(lr=params['learning_rate'], clipnorm=params.get('clipnorm', 1))
    model = Model(inputs=[inputs], outputs=[output])
    model.compile(loss='categorical_crossentropy', optimizer=optimizer, metrics=['accuracy'])
    return model