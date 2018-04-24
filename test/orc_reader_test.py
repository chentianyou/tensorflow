import tensorflow as tf

def read_file_format(filename_queue):
    # reader = tf.TextLineReader()
    reader = tf.OrcRowReader()
    key, value = reader.read(filename_queue)
    # record_defaults=[[0.0]]*9
    # cols = tf.decode_csv(value, record_defaults=record_defaults)
    # out_type = [tf.int32,tf.string]
    features = tf.parse_single_example(value, features={
        "key0":tf.FixedLenFeature([], tf.float32),
        "key1":tf.FixedLenFeature([], tf.float32),
        "key2":tf.FixedLenFeature([], tf.float32),
        "key3":tf.FixedLenFeature([], tf.float32),
        "key4":tf.FixedLenFeature([], tf.float32),
        "key5":tf.FixedLenFeature([], tf.float32),
        "key6":tf.FixedLenFeature([], tf.float32),
        "key7":tf.FixedLenFeature([], tf.float32),
        "key8":tf.FixedLenFeature([], tf.float32)
    })
    print(features)
    # out_type = [tf.float64]*9
    # cols = tf.decode_orc(value,out_type = out_type)
    # features = tf.stack(cols[0:-1])
    # label = tf.stack(cols[-1])

    return features

def input_pipeline(filenames, batch_size, num_epochs=None):
    filename_queue = tf.train.string_input_producer(
        filenames, num_epochs=num_epochs, shuffle=True)
    example = read_file_format(filename_queue)
    return example
    # min_after_dequeue = 0  # 10000
    # capacity = min_after_dequeue + 3 * batch_size
    # # example_batch, label_batch = tf.train.batch(
    # #     [example, label], batch_size=batch_size, num_threads=4, capacity=capacity)
    # example_batch, label_batch = tf.train.shuffle_batch(
    #     [example, label], batch_size=batch_size, capacity=capacity,
    #     min_after_dequeue=min_after_dequeue)
    # return example_batch, label_batch

files = ["hdfs://localhost:9000/hawq/dfs_default/postgres/public/housing_orc/W27627B74-3CA6-11E8-A2AB-FE008C60F601"]
# files = ["/Users/chentianyou/dev/tf_example/data/housing_features.csv"]
features = input_pipeline(files, 50, num_epochs=1)

# with tf.Session() as sess:
#     tf.global_variables_initializer().run()
#     tf.local_variables_initializer().run()
#     # Start populating the filename queue.
#     coord = tf.train.Coordinator()
#     threads = tf.train.start_queue_runners(coord=coord)
#     try:
#        step = 1
#        while not coord.should_stop():
#            batch_x = sess.run([features])
#            print(step,batch_x)
#            step += 1
#     except tf.errors.OutOfRangeError:
#        print('Done training, epoch reached')
#     finally:
#        coord.request_stop()
#     coord.join(threads)