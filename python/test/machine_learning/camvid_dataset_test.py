import os, sys
if 'posix' == os.name:
	swl_python_home_dir_path = '/home/sangwook/work/SWL_github/python'
else:
	swl_python_home_dir_path = 'D:/work/SWL_github/python'
sys.path.append(swl_python_home_dir_path + '/src')

from swl.machine_learning.camvid_dataset import create_camvid_generator, create_camvid_generator2, load_camvid_dataset

#%%------------------------------------------------------------------

if 'posix' == os.name:
	#dataset_home_dir_path = '/home/sangwook/my_dataset'
	dataset_home_dir_path = '/home/HDD1/sangwook/my_dataset'
else:
	dataset_home_dir_path = 'D:/dataset'

train_image_dir_path = dataset_home_dir_path + '/pattern_recognition/camvid/tmp/train/image'
train_label_dir_path = dataset_home_dir_path + '/pattern_recognition/camvid/tmp/trainannot/image'
val_image_dir_path = dataset_home_dir_path + '/pattern_recognition/camvid/tmp/val/image'
val_label_dir_path = dataset_home_dir_path + '/pattern_recognition/camvid/tmp/valannot/image'
test_image_dir_path = dataset_home_dir_path + '/pattern_recognition/camvid/tmp/test/image'
test_label_dir_path = dataset_home_dir_path + '/pattern_recognition/camvid/tmp/testannot/image'

image_suffix = ''
image_extension = 'png'
label_suffix = ''
label_extension = 'png'

num_examples = 367
num_classes = 12

batch_size = 32
shuffle = False

#%%------------------------------------------------------------------
# Create a dataset generator.

original_image_size = (360, 480)  # (height, width).
resized_image_size = None
#resized_image_size = original_image_size
random_crop_size = None
#random_crop_size = (224, 224)  # (height, width).
center_crop_size = None

if center_crop_size is not None:
	image_size = center_crop_size
elif random_crop_size is not None:
	image_size = random_crop_size
elif resized_image_size is not None:
	image_size = resized_image_size
else:
	image_size = original_image_size
image_shape = image_size + (3,)

use_loaded_dataset = True

# Provide the same seed and keyword arguments to the fit and flow methods.
seed = 1

#train_dataset_gen, val_dataset_gen, test_dataset_gen = create_camvid_generator(
#		train_image_dir_path, train_label_dir_path, val_image_dir_path, val_label_dir_path, test_image_dir_path, test_label_dir_path,
#		num_classes, batch_size=batch_size,
#		data_suffix=image_suffix, data_extension=image_extension, label_suffix=label_suffix, label_extension=label_extension,
#		resized_image_size=resized_image_size, random_crop_size=random_crop_size, center_crop_size=center_crop_size, use_loaded_dataset=use_loaded_dataset, shuffle=shuffle, seed=seed)
train_dataset_gen, val_dataset_gen, test_dataset_gen = create_camvid_generator2(
		train_image_dir_path, train_label_dir_path, val_image_dir_path, val_label_dir_path, test_image_dir_path, test_label_dir_path,
		num_classes, batch_size=batch_size,
		data_suffix=image_suffix, data_extension=image_extension, label_suffix=label_suffix, label_extension=label_extension,
		width=image_shape[1], height=image_shape[0], shuffle=shuffle)

# Usage.
#num_examples = 367
#num_epochs = 10
#steps_per_epoch = num_examples / batch_size
#model.fit_generator(train_dataset_gen, steps_per_epoch=steps_per_epoch, epochs=num_epochs)
#for epoch in range(num_epochs):
#	print('Epoch', epoch)
#	num_batches = 0
#	for data_batch, label_batch in train_dataset_gen:
#		model.fit(data_batch, label_batch)
#		num_batches += 1
#		if num_batches >= steps_per_epoch:
#			break

#%%------------------------------------------------------------------
# Load images and convert them to numpy.array.

image_width, image_height = None, None
#image_width, image_height = 480, 360

train_images, train_labels, val_images, val_labels, test_images, test_labels = load_camvid_dataset(
		train_image_dir_path, train_label_dir_path, val_image_dir_path, val_label_dir_path, test_image_dir_path, test_label_dir_path,
		data_suffix=image_suffix, data_extension=image_extension, label_suffix=label_suffix, label_extension=label_extension,
		width=image_width, height=image_height)

# Usage.
#num_examples = 367
#batch_size = 32
#num_epochs = 10
#history = model.fit(train_images, train_labels, batch_size=batch_size, epochs=num_epochs)

#%%------------------------------------------------------------------
# For comparison.

import numpy as np

image_width, image_height = 480, 360
num_epochs = 1
steps_per_epoch = num_examples / batch_size

data_list = []
labels_list = []
for epoch in range(num_epochs):
	print('Epoch', epoch)
	num_batches = 0
	for data_batch, label_batch in train_dataset_gen:
		data_list.append(data_batch)
		labels_list.append(label_batch)
		num_batches += 1
		if num_batches >= steps_per_epoch:
			break

assert len(data_list) == len(labels_list), '[Error] len(data_list) == len(labels_list)'
generated_images = np.ndarray(shape=(num_examples, image_height, image_width, 3))
generated_labels = np.ndarray(shape=(num_examples, image_height, image_width, num_classes))
for idx in range(len(data_list)):
	start_idx = idx * batch_size
	end_idx = start_idx + data_list[idx].shape[0]
	generated_images[start_idx:end_idx] = data_list[idx]
	generated_labels[start_idx:end_idx] = labels_list[idx]
generated_labels = generated_labels.astype(np.uint8)

np.sum(train_images - generated_images)
np.sum(train_labels - generated_labels)

#%%------------------------------------------------------------------
# Plot.

import matplotlib.pyplot as plt

idx = 0
plt.figure()
plt.imshow((train_images[idx] - np.min(train_images[idx])) / (np.max(train_images[idx]) - np.min(train_images[idx])))
plt.figure()
plt.imshow((generated_images[idx] - np.min(generated_images[idx])) / (np.max(generated_images[idx]) - np.min(generated_images[idx])))
