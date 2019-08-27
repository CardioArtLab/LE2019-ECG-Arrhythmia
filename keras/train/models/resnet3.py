from keras import backend as K
from keras.models import Model
from keras.layers import Input, Conv1D, BatchNormalization, Activation, Dense, Add, MaxPool1D, Dropout, ZeroPadding1D, Reshape
from keras.layers.core import Lambda
from keras.layers.wrappers import TimeDistributed
from keras.optimizers import Adam

def zeropad(x):
        y = K.zeros_like(x)
        return K.concatenate([x, y], axis=2)

def zeropad_output_shape(input_shape):
    shape = list(input_shape)
    assert len(shape) == 3
    shape[2] *= 2
    return tuple(shape)

'''
Build model
'''
def build_model(**params):
    inputs = Input(shape=params['input_size'], dtype='float32', name='inputs')
    layer = Conv1D(params['conv_num_filters_start'], params['conv_filter_length'], padding='same', kernel_initializer=params['conv_init'])(inputs)
    layer = BatchNormalization()(layer)
    layer = Activation(params['conv_activation'])(layer)
    for index, subsample_length in enumerate(params['conv_subsample_lengths']):
        num_filters = 2**int(index/params['conv_increase_channels_at']) * params['conv_num_filters_start']
        shortcut = MaxPool1D(pool_size=subsample_length)(layer)
        is_zero_pad = (index % params['conv_increase_channels_at'] ) == 0 and index > 0
        if is_zero_pad:
            #shortcut = ZeroPadding1D((0, 0))(shortcut)
            shortcut = Lambda(zeropad, output_shape=zeropad_output_shape)(shortcut)
        for i in range(params['conv_num_skip']):
            if not (index == 0 and i==0):
                layer = BatchNormalization()(layer)
                layer = Activation(params['conv_activation'])(layer)
                if i > 0:
                    layer = Dropout(params['conv_dropout'])(layer)
            layer = Conv1D(num_filters, params['conv_filter_length'], strides=subsample_length if i==0 else 1, padding='same', kernel_initializer=params['conv_init'])(layer)
        layer = Add()([shortcut, layer])
    layer = BatchNormalization()(layer)
    layer = Activation(params['conv_activation'])(layer)
    layer = Reshape((1, params['conv_last_size']))(layer)
    layer = TimeDistributed(Dense(params['output_size']))(layer)
    output = Activation('softmax')(layer)
    # customize optimizer
    optimizer = Adam(lr=params['learning_rate'], clipnorm=params.get('clipnorm', 1))
    model = Model(inputs=[inputs], outputs=[output])
    model.compile(loss='categorical_crossentropy', optimizer=optimizer, metrics=['accuracy'])
    return model