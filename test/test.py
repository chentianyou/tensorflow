import time
import os

print(os.getpid())
for i in range(12):
    print(i)
    time.sleep(1)
import tensorflow as tf

with tf.Session() as sess:
    print(tf.__version__)