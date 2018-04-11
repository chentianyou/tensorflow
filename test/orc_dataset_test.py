import tensorflow as tf
import numpy as np
import sys

tf.reset_default_graph()

batch_size = 20
learning_rate = 0.01
layers = [100,50,20]
num_epochs = 50

import time
start = time.clock()

from sklearn.preprocessing import StandardScaler
scaler = StandardScaler()

orc_files = ["/Users/chentianyou/dev/tensorflow/test/data/W27627B74-3CA6-11E8-A2AB-FE008C60F601"]

def orc_input_fn(files):
    dataset = tf.data.ORCFileDataset(files)
    def orc_decode(record):
        out_type = [tf.float64] * 9
        cols = tf.decode_orc(record, out_type = out_type)
        features = tf.stack(cols[0:-1])
        labels = tf.stack([cols[-1]])
        return {'Feature': features},labels

    # def whole_parser_pd(dataset):
    #     iterator = dataset.make_one_shot_iterator()
    #     next_element = iterator.get_next()
    #     out_type = [tf.float64] * 9
    #     cols = tf.decode_orc(next_element, out_type = out_type)
    #     features = tf.stack(cols[0:-1])
    #     labels = tf.stack([cols[-1]])
    #     features = tf.reshape(features,shape=(-1,8))
    #     labels = tf.reshape(labels,shape=(-1,1))
    #     print(features, labels)
    #     return tf.data.Dataset.from_tensor_slices(({'Feature': features}, labels))
    # dataset = dataset.skip(1).apply(whole_parser_pd)
    dataset = dataset.map(orc_decode)
    dataset = dataset.shuffle(buffer_size=10000)
    dataset = dataset.batch(batch_size=batch_size)
    dataset = dataset.repeat(num_epochs)

    iterator = dataset.make_one_shot_iterator()
    features, labels = iterator.get_next()
    return features, labels

class DNNRegressorWrapper:
    def __init__(self, **params):
        self.estimator = tf.estimator.DNNRegressor(params)
        
    def get_params(self, deep):
        params = self.estimator.get_params(deep)
        return params['params']
    
    def set_param(self, **parma):
        self.estimator.set_param(param)
        
    def fit(self, X, y=None, input_fn=None, steps=None, batch_size=None, monitors=None, max_steps=None):
        self.estimator.fit(X, y, input_fn, steps, batch_size)

feature_columns = []
feature_columns.append(tf.feature_column.numeric_column(key='Feature', shape=(8, 1)))

#tf.contrib.learn.DNNRegressor also supports DataSet !
dnn_housing = tf.contrib.learn.DNNRegressor(hidden_units=layers, 
#dnn_housing = DNNRegressorWrapper(hidden_units=layers, 
                              feature_columns=feature_columns,
                              activation_fn=tf.nn.relu,
                              optimizer=tf.train.AdamOptimizer(learning_rate=learning_rate))

dnn_housing.fit(x=None, input_fn=lambda:orc_input_fn(orc_files))

end = time.clock()
print(end-start)

data = [[2.3447657583017163,0.9821426581785077,0.6285594533305325,-0.15375758957963684,-0.9744285971768408,
        -0.04959653614560168,1.0525482830366848,-1.3278352216308462],
        [2.3322379635373314,-0.6070189133741593,0.32704135754480507,-0.26333577077119036,0.8614388682720688,-0.09251223020604675,1.043184551645693,-1.3228439144608317],
        [1.7826994032844994,1.8561815225324745,1.1556204656817255,-0.04901635892839476,-0.8207773518723145,-0.025842527794874764,1.038502685950199,-1.3328265288008536],
        [0.9329675088245888,1.8561815225324745,0.1569660820761381,-0.04983291824965424,-0.7660280575684029,-0.05032930122595976,1.038502685950199,-1.3378178359708683],
        [-0.9423591469347639,1.0616007367561409,-0.45870257434508765,0.04425392871991308,-0.19380962677913224,-0.10049920162528048,1.0338208202547048,-1.3428091431408828],
        [-1.1609112640008328,1.538349208221941,-0.26741705368289886,-0.06294633695379152,-0.4304678666734602,-0.05417337900045864,1.647145226364561,-1.0083915627500772]]

def getTestData(x):
    dataset = tf.data.Dataset.from_tensor_slices({'Feature' : x})
    return dataset.batch(500)

#pred = dnn_housing.predict_scores(lambda:getTestData(data))

# pred = dnn_housing.predict_scores(x=None, input_fn=lambda:getTestData(data), as_iterable=False)
pred = dnn_housing.predict_scores({'Feature' : np.array(data)}, as_iterable=False)
print(pred)

end = time.clock()
print("run time: ", end-start)