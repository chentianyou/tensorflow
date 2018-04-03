import tensorflow as tf

def read_file_format(filename_queue):
    reader = tf.OrcRowReader()
    key, value = reader.read(filename_queue)
    # record_defaults=[[0.0]]*9
    # cols = tf.decode_csv(value, record_defaults=record_defaults)
    out_type = [tf.float64]*9
    cols = tf.decode_orc(value,out_tpye = out_type)
    features = tf.stack(cols[0:-1])
    label = tf.stack(cols[-1])
    return features, label

def input_pipeline(filenames, batch_size, num_epochs=None):
    filename_queue = tf.train.string_input_producer(
        filenames, num_epochs=num_epochs, shuffle=True)
    example, label = read_file_format(filename_queue)
    return example, label
    # min_after_dequeue = 0  # 10000
    # capacity = min_after_dequeue + 3 * batch_size
    # # example_batch, label_batch = tf.train.batch(
    # #     [example, label], batch_size=batch_size, num_threads=4, capacity=capacity)
    # example_batch, label_batch = tf.train.shuffle_batch(
    #     [example, label], batch_size=batch_size, capacity=capacity,
    #     min_after_dequeue=min_after_dequeue)
    # return example_batch, label_batch

files = ["/Users/chentianyou/dev/tensorflow/test/data/W1F87D214-281F-11E8-BE8A-8C85904DEC7F"]
#files = ["/Users/chentianyou/dev/tf_example/data/housing_features.csv"]
features, target = input_pipeline(files, 50, num_epochs=1)

with tf.Session() as sess:
    tf.global_variables_initializer().run()
    tf.local_variables_initializer().run()
    # Start populating the filename queue.
    coord = tf.train.Coordinator()
    threads = tf.train.start_queue_runners(coord=coord)
    try:
       step = 0
       while not coord.should_stop():
           batch_x, batch_y = sess.run([features, target])
           print(batch_x)
           print(batch_y)
    except tf.errors.OutOfRangeError:
       print('Done training, epoch reached')
    finally:
       coord.request_stop()
    coord.join(threads)