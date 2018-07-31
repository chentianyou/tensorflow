import os
import sys
import time

import numpy as np
import tensorflow as tf
from sklearn.preprocessing import StandardScaler

batch_size = 50
learning_rate = 0.01
layers = [100, 50, 20]
num_epochs = 50

scaler = StandardScaler()
orc_files = [os.path.join(sys.path[0], "data/W27627B74-3CA6-11E8-A2AB-FE008C60F601")]
start = time.clock()

def orc_input_fn(files):
    dataset = tf.data.ORCFileDataset(files)

    def orc_decode(record):
        features = tf.parse_example(record, features={
            "key0": tf.FixedLenFeature([], tf.float32),
            "key1": tf.FixedLenFeature([], tf.float32),
            "key2": tf.FixedLenFeature([], tf.float32),
            "key3": tf.FixedLenFeature([], tf.float32),
            "key4": tf.FixedLenFeature([], tf.float32),
            "key5": tf.FixedLenFeature([], tf.float32),
            "key6": tf.FixedLenFeature([], tf.float32),
            "key7": tf.FixedLenFeature([], tf.float32),
            "key8": tf.FixedLenFeature([], tf.float32),
        })
        labels = features["key8"]
        features = dict([("key%d" % i, features["key%d" % i]) for i in range(8)])
        return features, labels

    dataset = dataset.shuffle(buffer_size=10000)
    dataset = dataset.batch(batch_size=batch_size)
    dataset = dataset.repeat(num_epochs)
    dataset = dataset.map(orc_decode)

    iterator = dataset.make_one_shot_iterator()
    features, labels = iterator.get_next()
    return features, labels


if __name__ == '__main__':
    feature_columns = []
    for i in range(8):
        feature_columns.append(tf.feature_column.numeric_column(key='key%d' % i))

    features, labels = orc_input_fn(orc_files)

    with tf.Session() as sess:
        while True:
            try:
                print(sess.run(features))
            except tf.errors.OutOfRangeError:
                break
