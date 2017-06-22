# REF [paper] >> "Densely Connected Convolutional Networks", arXiv 2016.
# REF [paper] >> "The One Hundred Layers Tiramisu: Fully Convolutional DenseNets for Semantic Segmentation", arXiv 2016.
# REF [site] >> https://github.com/titu1994/Fully-Connected-DenseNets-Semantic-Segmentation

#%%------------------------------------------------------------------

import os
if 'posix' == os.name:
	swl_python_home_dir_path = '/home/sangwook/work/SWL_github/python'
else:
	swl_python_home_dir_path = 'D:/work/SWL_github/python'
os.chdir(swl_python_home_dir_path + '/test/machine_learning/keras')

#lib_home_dir_path = "/home/sangwook/lib_repo/python"
lib_home_dir_path = "D:/lib_repo/python"
#lib_home_dir_path = "D:/lib_repo/python/rnd"

lib_dir_path = lib_home_dir_path + "/Fully-Connected-DenseNets-Semantic-Segmentation_github"

import sys
sys.path.append('../../../src')
sys.path.append(lib_dir_path)

#%%------------------------------------------------------------------

import numpy as np
import tensorflow as tf
import keras
from keras import backend as K
import densenet_fc as dc
from swl.machine_learning.keras.preprocessing import ImageDataGeneratorWithCrop
from swl.machine_learning.data_loader import DataLoader

#%%------------------------------------------------------------------

config = tf.ConfigProto()
#config.allow_soft_placement = True
config.log_device_placement = True
config.gpu_options.allow_growth = True
#config.gpu_options.per_process_gpu_memory_fraction = 0.4  # only allocate 40% of the total memory of each GPU.
sess = tf.Session(config=config)

# This means that Keras will use the session we registered to initialize all variables that it creates internally.
K.set_session(sess)
K.set_learning_phase(0)

#%%------------------------------------------------------------------
# Load data.

#dataset_home_dir_path = "/home/sangwook/my_dataset"
#dataset_home_dir_path = "/home/HDD1/sangwook/my_dataset"
dataset_home_dir_path = "D:/dataset"

train_data_dir_path = dataset_home_dir_path + "/pattern_recognition/camvid/tmp/train"
train_label_dir_path = dataset_home_dir_path + "/pattern_recognition/camvid/tmp/trainannot"
validation_data_dir_path = dataset_home_dir_path + "/pattern_recognition/camvid/tmp/val"
validation_label_dir_path = dataset_home_dir_path + "/pattern_recognition/camvid/tmp/valannot"
test_data_dir_path = dataset_home_dir_path + "/pattern_recognition/camvid/tmp/test"
test_label_dir_path = dataset_home_dir_path + "/pattern_recognition/camvid/tmp/testannot"

# NOTICE [caution] >>
#	If the size of data is changed, labels may be dense.

data_loader = DataLoader()
#data_loader = DataLoader(width=480, height=360)
train_dataset = data_loader.load(data_dir_path=train_data_dir_path, label_dir_path=train_label_dir_path, data_extension ='png', label_extension='png')
validation_dataset = data_loader.load(data_dir_path=validation_data_dir_path, label_dir_path=validation_label_dir_path, data_extension ='png', label_extension='png')
test_dataset = data_loader.load(data_dir_path=test_data_dir_path, label_dir_path=test_label_dir_path, data_extension ='png', label_extension='png')

# Change the dimension of labels.
if train_dataset.data.ndim == train_dataset.labels.ndim:
	pass
elif 1 == train_dataset.data.ndim - train_dataset.labels.ndim:
	train_dataset.labels = train_dataset.labels.reshape(train_dataset.labels.shape + (1,))
else:
	raise ValueError('train_dataset.data.ndim or train_dataset.labels.ndim is invalid.')
if validation_dataset.data.ndim == validation_dataset.labels.ndim:
	pass
elif 1 == validation_dataset.data.ndim - validation_dataset.labels.ndim:
	validation_dataset.labels = validation_dataset.labels.reshape(validation_dataset.labels.shape + (1,))
else:
	raise ValueError('validation_dataset.data.ndim or validation_dataset.labels.ndim is invalid.')
if test_dataset.data.ndim == test_dataset.labels.ndim:
	pass
elif 1 == test_dataset.data.ndim - test_dataset.labels.ndim:
	test_dataset.labels = test_dataset.labels.reshape(test_dataset.labels.shape + (1,))
else:
	raise ValueError('test_dataset.data.ndim or test_dataset.labels.ndim is invalid.')

#%%------------------------------------------------------------------

keras_backend = 'tf'

num_examples = train_dataset.num_examples
num_classes = np.unique(train_dataset.labels).shape[0]  # 11 + 1.

batch_size = 3
num_epochs = 50
steps_per_epoch = num_examples // batch_size
if steps_per_epoch < 1:
	steps_per_epoch = 1

resized_input_size = train_dataset.data.shape[1:3]  # (height, width) = (360, 480).
cropped_input_size = (224, 224)  # (height, width).

train_data_shape = (None,) + cropped_input_size + (train_dataset.data.shape[3],)
#train_label_shape = (None,) + cropped_input_size + (train_dataset.labels.shape[3],)
train_label_shape = (None,) + cropped_input_size + (1 if 2 == num_classes else num_classes,)
train_data_tf = tf.placeholder(tf.float32, shape=train_data_shape)
train_labels_tf = tf.placeholder(tf.uint8, shape=train_label_shape)

#%%------------------------------------------------------------------
# Create a data generator.

# REF [site] >> https://keras.io/preprocessing/image/
# REF [site] >> https://github.com/fchollet/keras/issues/3338

train_data_generator = ImageDataGeneratorWithCrop(
	rescale=1./255.,
	preprocessing_function=None,
	featurewise_center=False,
	featurewise_std_normalization=False,
	samplewise_center=False,
	samplewise_std_normalization=False,
	zca_whitening=False,
	zca_epsilon=1e-6,
	#rotation_range=20,
	#width_shift_range=0.2,
	#height_shift_range=0.2,
	#horizontal_flip=True,
	vertical_flip=True,
	#zoom_range=0.2,
	#shear_range=0.,
	#channel_shift_range=0.,
	random_crop_size=cropped_input_size,
	center_crop_size=None,
	fill_mode='reflect',
	cval=0.)
train_label_generator = ImageDataGeneratorWithCrop(
	#rescale=1./255.,
	#preprocessing_function=None,
	#featurewise_center=False,
	#featurewise_std_normalization=False,
	#samplewise_center=False,
	#samplewise_std_normalization=False,
	#zca_whitening=False,
	#zca_epsilon=1e-6,
	#rotation_range=20,
	#width_shift_range=0.2,
	#height_shift_range=0.2,
	#horizontal_flip=True,
	vertical_flip=True,
	#zoom_range=0.2,
	#shear_range=0.,
	#channel_shift_range=0.,
	random_crop_size=cropped_input_size,
	center_crop_size=None,
	fill_mode='reflect',
	cval=0.)
test_data_generator = ImageDataGeneratorWithCrop(rescale=1./255.)
test_label_generator = ImageDataGeneratorWithCrop()

# Provide the same seed and keyword arguments to the fit and flow methods.
seed = 1

# Compute the internal data stats related to the data-dependent transformations, based on an array of sample data.
# Only required if featurewise_center or featurewise_std_normalization or zca_whitening.
#train_data_generator.fit(train_dataset.data, augment=True, seed=seed)
##train_label_generator.fit(train_dataset.labels, augment=True, seed=seed)

train_data_gen = train_data_generator.flow_from_directory(
	train_data_dir_path,
	target_size=resized_input_size,
	color_mode='rgb',
	#classes=None,
	class_mode=None,  # NOTICE [important] >>
	batch_size=batch_size,
	shuffle=True,
	seed=seed)
train_label_gen = train_label_generator.flow_from_directory(
	train_label_dir_path,
	target_size=resized_input_size,
	color_mode='grayscale',
	#classes=None,
	class_mode=None,  # NOTICE [important] >>
	batch_size=batch_size,
	shuffle=True,
	seed=seed)
validation_data_gen = test_data_generator.flow_from_directory(
	validation_data_dir_path,
	target_size=resized_input_size,
	color_mode='rgb',
	#classes=None,
	class_mode=None,  # NOTICE [important] >>
	batch_size=batch_size,
	shuffle=True,
	seed=seed)
validation_label_gen = test_label_generator.flow_from_directory(
	validation_label_dir_path,
	target_size=resized_input_size,
	color_mode='grayscale',
	#classes=None,
	class_mode=None,  # NOTICE [important] >>
	batch_size=batch_size,
	shuffle=True,
	seed=seed)
test_data_gen = test_data_generator.flow_from_directory(
	test_data_dir_path,
	target_size=resized_input_size,
	color_mode='rgb',
	#classes=None,
	class_mode=None,  # NOTICE [important] >>
	batch_size=batch_size,
	shuffle=True,
	seed=seed)
test_label_gen = test_label_generator.flow_from_directory(
	test_label_dir_path,
	target_size=resized_input_size,
	color_mode='grayscale',
	#classes=None,
	class_mode=None,  # NOTICE [important] >>
	batch_size=batch_size,
	shuffle=True,
	seed=seed)

# Combine generators into one which yields image and labels.
train_dataset_gen = zip(train_data_gen, train_label_gen)
validation_dataset_gen = zip(validation_data_gen, validation_label_gen)
test_dataset_gen = zip(test_data_gen, test_label_gen)

#%%------------------------------------------------------------------
# Create a FC-DenseNet model.

with tf.name_scope('fc-densenet'):
	fc_densenet_model = dc.DenseNetFCN(train_data_shape[1:], nb_dense_block=5, growth_rate=16, nb_layers_per_block=4, upsampling_type='upsampling', classes=num_classes)
	fc_densenet_model.summary()

fc_densenet_model_output = fc_densenet_model(train_data_tf)

#%%------------------------------------------------------------------
# Configure training parameters of the FC-DenseNet model.

# Use Keras ==> Cannot train.
#fc_densenet_model.fit_generator(
#	train_generator,
#	steps_per_epoch=steps_per_epoch,
#	epochs=num_epochs,
#	validation_data=validation_generator,
#	validation_steps=800)

# Use TensorFlow.
# REF [site] >> https://blog.keras.io/keras-as-a-simplified-interface-to-tensorflow-tutorial.html

# Define a loss.
with tf.name_scope('loss'):
	#loss = tf.reduce_mean(keras.objectives.categorical_crossentropy(train_labels_tf, fc_densenet_model_output))
	loss = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=train_labels_tf, logits=fc_densenet_model_output))

# Define an optimzer.
global_step = tf.Variable(0, name='global_step', trainable=False)
with tf.name_scope('learning_rate'):
	learning_rate = tf.train.exponential_decay(1.0e-3, global_step, 1, 0.995, staircase=True)
train_step = tf.train.RMSPropOptimizer(learning_rate=learning_rate).minimize(loss, global_step=global_step)

#%%------------------------------------------------------------------
# Train the FC-DenseNet model.

# Initialize all variables.
sess.run(tf.global_variables_initializer())

# Run training loop.
with sess.as_default():
	for epoch in range(num_epochs):
		print('Epoch %d/%d' % (epoch + 1, num_epochs))
		steps = 0
		for data_batch, label_batch in train_dataset_gen:
			label_batch = keras.utils.to_categorical(label_batch, num_classes).reshape(label_batch.shape[:-1] + (-1,)).astype(np.uint8)
			#print('data batch: (shape, dtype, min, max) =', data_batch.shape, data_batch.dtype, np.min(data_batch), np.max(data_batch))
			#print('label batch: (shape, dtype, min, max) =', label_batch.shape, label_batch.dtype, np.min(label_batch), np.max(label_batch))
			train_step.run(feed_dict={train_data_tf: data_batch, train_labels_tf: label_batch})
			steps += 1
			if steps >= steps_per_epoch:
				break

#%%------------------------------------------------------------------
# Evaluate the FC-DenseNet model.

acc_value = keras.metrics.categorical_accuracy(train_labels_tf, fc_densenet_model_output)
with sess.as_default():
	print(acc_value.eval(feed_dict={train_data_tf: data_batch, train_labels_tf: label_batch}))
