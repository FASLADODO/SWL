import os, time
from functools import partial
import multiprocessing as mp
from multiprocessing.managers import BaseManager
import threading
import numpy as np
#import imgaug as ia
from imgaug import augmenters as iaa
from swl.machine_learning.imgaug_data_generator import ImgaugDataGenerator
from swl.machine_learning.batch_generator import NpzFileBatchGeneratorWithNpyFileInput
from swl.machine_learning.batch_loader import NpzFileBatchLoader
from swl.util.working_directory_manager import WorkingDirectoryManager, TwoStepWorkingDirectoryManager
import swl.util.util as swl_util
import swl.machine_learning.util as swl_ml_util

#%%------------------------------------------------------------------
# Synth90kDataset

class Synth90kDataset(object):
	def __init__(self):
		# NOTE [info] >> The same parameters exist in class Synth90kLabelConverter in ${SWL_PYTHON_HOME}/test/language_processing/synth90k_dataset_test.py.

		self._image_height, self._image_width, self._image_channel = 32, 128, 1
		self._max_label_len = 23  # Max length of words in lexicon.

		# Label: 0~9 + a~z + A~Z.
		#label_characters = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
		# Label: 0~9 + a~z.
		label_characters = '0123456789abcdefghijklmnopqrstuvwxyz'

		self._SOS = '<SOS>'  # All strings will start with the Start-Of-String token.
		self._EOS = '<EOS>'  # All strings will end with the End-Of-String token.
		#extended_label_list = [SOS] + list(label_characters) + [EOS]
		extended_label_list = list(label_characters) + [EOS]
		#extended_label_list = list(label_characters)

		#self._label_int2char = extended_label_list
		self._label_char2int = {c:i for i, c in enumerate(extended_label_list)}

		self._num_labels = len(extended_label_list)
		self._num_classes = self._num_labels + 1  # Extended labels + blank label.
		# NOTE [info] >> The largest value (num_classes - 1) is reserved for the blank label.
		self._blank_label = self._num_classes - 1
		self._label_eos_token = self._label_char2int[EOS]

	@property
	def label_size(self):
		return self._num_labels

	@property
	def max_token_len(self):
		return self._max_label_len

	@property
	def start_token(self):
		return self._label_char2int[self._SOS]

	@property
	def end_token(self):
		return self._label_char2int[self._EOS]

#%%------------------------------------------------------------------
# ImgaugDataAugmenter.

class ImgaugDataAugmenter(object):
	def __init__(self, is_output_augmented=False):
		self._augment_functor = self._augmentWithOutputAugmentation if is_output_augmented else self._augmentWithoutOutputAugmentation
		self._augmenter = iaa.Sequential([
			iaa.SomeOf(1, [
				#iaa.Sometimes(0.5, iaa.Crop(px=(0, 100))),  # Crop images from each side by 0 to 16px (randomly chosen).
				iaa.Sometimes(0.5, iaa.Crop(percent=(0, 0.1))), # Crop images by 0-10% of their height/width.
				iaa.Fliplr(0.1),  # Horizontally flip 10% of the images.
				iaa.Flipud(0.1),  # Vertically flip 10% of the images.
				iaa.Sometimes(0.5, iaa.Affine(
					scale={'x': (0.8, 1.2), 'y': (0.8, 1.2)},  # Scale images to 80-120% of their size, individually per axis.
					translate_percent={'x': (-0.2, 0.2), 'y': (-0.2, 0.2)},  # Translate by -20 to +20 percent (per axis).
					rotate=(-45, 45),  # Rotate by -45 to +45 degrees.
					shear=(-16, 16),  # Shear by -16 to +16 degrees.
					#order=[0, 1],  # Use nearest neighbour or bilinear interpolation (fast).
					order=0,  # Use nearest neighbour or bilinear interpolation (fast).
					#cval=(0, 255),  # If mode is constant, use a cval between 0 and 255.
					#mode=ia.ALL  # Use any of scikit-image's warping modes (see 2nd image from the top for examples).
					#mode='edge'  # Use any of scikit-image's warping modes (see 2nd image from the top for examples).
				)),
				iaa.Sometimes(0.5, iaa.GaussianBlur(sigma=(0, 3.0)))  # Blur images with a sigma of 0 to 3.0.
			]),
			#iaa.Scale(size={'height': image_height, 'width': image_width})  # Resize.
		])

	def __call__(self, inputs, outputs, *args, **kwargs):
		return self._augment_functor(inputs, outputs, *args, **kwargs)

	def _augmentWithoutOutputAugmentation(self, inputs, outputs, *args, **kwargs):
		return self._augmenter.augment_images(inputs), outputs

	def _augmentWithOutputAugmentation(self, inputs, outputs, *args, **kwargs):
		augmenter_det = self._augmenter.to_deterministic()  # Call this for each batch again, NOT only once at the start.
		return augmenter_det.augment_images(inputs), augmenter_det.augment_images(outputs)

#%%------------------------------------------------------------------
# Synth90kDataPreprocessor.

class Synth90kDataPreprocessor(object):
	def __init__(self, num_classes, label_eos_token, is_sparse_output):
		self._num_classes = num_classes.
		self._label_eos_token = label_eos_token
		self._is_sparse_output = is_sparse_output

	def __call__(self, inputs, outputs, *args, **kwargs):
		"""
		Inputs:
			inputs (numpy.array): Images of size (samples, height, width) and type uint8.
			outputs (numpy.array): Labels of size (samples, max_label_length) and type uint8.
		Outputs:
			inputs (numpy.array): Images of size (samples, height, width, 1) and type float32.
			outputs (numpy.array or a tuple): Labels of size (samples, max_label_length, num_labels) and type uint8 when is_sparse_output = False. A tuple with (indices, values, shape) for a sparse tensor when is_sparse_output = True.
		"""

		if inputs is not None:
			# inputs' shape = (32, 128) -> (32, 128, 1).

			# Preprocessing (normalization, standardization, etc.).
			inputs = np.reshape(inputs.astype(np.float32) / 255.0, inputs.shape + (1,))
			#inputs = (inputs - np.mean(inputs, axis=axis)) / np.std(inputs, axis=axis)

		if outputs is not None:
			if self._is_sparse_output:
				# Sparse tensor: (num_examples, max_label_len) -> A tuple with (indices, values, shape) for a sparse tensor.
				outputs = swl_ml_util.dense_to_sparse(outputs, self._label_eos_token, np.uint8)
			else:
				# One-hot encoding: (num_examples, max_label_len) -> (num_examples, max_label_len, num_classes).
				outputs = swl_ml_util.to_one_hot_encoding(outputs, self._num_classes).astype(np.uint8)
				#outputs = swl_ml_util.to_one_hot_encoding(outputs, self._num_classes).astype(np.uint8)  # Error.

		return inputs, outputs

#%%------------------------------------------------------------------
# Synth90kDataGenerator.

class Synth90kDataGenerator(ImgaugDataGenerator):
	def __init__(self, is_sparse_output, is_output_augmented=False, is_augmented_in_parallel=True):
		super().__init__()

		self._dataset = Synth90kDataset()

		#--------------------
		self._is_augmented_in_parallel = is_augmented_in_parallel

		self._preprocessor = Synth90kDataPreprocessor(self._dataset._num_classes, self._dataset._label_eos_token, is_sparse_output)
		self._augmenter = ImgaugDataAugmenter(is_output_augmented)
		#self._augmenter = None

		#--------------------
		train_batch_dir_path_prefix = './train_batch_dir'
		num_train_batch_dirs = 10
		val_batch_dir_path_prefix = './val_batch_dir'
		num_val_batch_dirs = 1
		test_batch_dir_path_prefix = './test_batch_dir'
		num_test_batch_dirs = 1
		self._batch_info_csv_filename = 'batch_info.csv'

		self._num_loaded_files_at_a_time = 5

		self._trainDirMgr = TwoStepWorkingDirectoryManager(train_batch_dir_path_prefix, num_train_batch_dirs)
		self._valDirMgr = WorkingDirectoryManager(val_batch_dir_path_prefix, num_val_batch_dirs)
		self._testDirMgr = WorkingDirectoryManager(test_batch_dir_path_prefix, num_test_batch_dirs)

		self._trainFileBatchLoader = NpzFileBatchLoader(self._batch_info_csv_filename, data_processing_functor=self._preprocessor)
		self._valFileBatchLoader = NpzFileBatchLoader(self._batch_info_csv_filename, data_processing_functor=self._preprocessor)
		self._testFileBatchLoader = NpzFileBatchLoader(self._batch_info_csv_filename, data_processing_functor=self._preprocessor)

		self._isAugmentationThreadStarted = False
		self._isValidationBatchesGenerated, self._isTestBatchesGenerated = False, False

		#--------------------
		# Prepares multiprocessing.

		# set_start_method() should not be used more than once in the program.
		#mp.set_start_method('spawn')

		#BaseManager.register('WorkingDirectoryManager', WorkingDirectoryManager)
		BaseManager.register('TwoStepWorkingDirectoryManager', TwoStepWorkingDirectoryManager)
		#BaseManager.register('NpzFileBatchGeneratorWithNpyFileInput', NpzFileBatchGeneratorWithNpyFileInput)
		#BaseManager.register('NpzFileBatchLoader', NpzFileBatchLoader)
		self._manager = BaseManager()
		self._manager.start()

		self._lock = mp.Lock()
		#self._lock = mp.Manager().Lock()  # TypeError: can't pickle thread.lock objects.
		num_processes = 5

		self._augmentation_worker_thread = threading.Thread(target=Synth90kDataGenerator.augmentation_worker_thread_proc, args=(num_processes, self._manager, self._lock, train_batch_dir_path_prefix, num_train_batch_dirs, self._augmenter, self._train_input_filepaths, self._train_output_filepaths, self._num_loaded_files_at_a_time, batch_size, shuffle, self._batch_info_csv_filename))

	@property
	def dataset(self):
		if self._dataset is None:
			raise ValueError('Dataset is None')
		return self._dataset

	@property
	def shapes(self):
		if self._dataset is None:
			raise ValueError('Dataset is None')
		return self._dataset._image_height, self._dataset._image_width, self._dataset._image_channel, self._dataset._num_classes

	def initialize(self):
		# NOTE [info] >> Generate synth90k dataset using swl.language_processing.synth90k_dataset.save_synth90k_dataset_to_npy_files().
		#	Refer to ${SWL_PYTHON_HOME}/test/language_processing/synth90k_dataset_test.py.

		synth90k_base_dir_path = './synth90k_npy'
		self._train_input_filepaths, self._train_output_filepaths, self._val_input_filepaths, self._val_output_filepaths, self._test_input_filepaths, self._test_output_filepaths = Synth90kDataGenerator._loadDataFromNpyFiles(synth90k_base_dir_path)

		if len(self._train_input_filepaths) != len(self._train_output_filepaths):
			raise ValueError('The lengths of train input and output data are different: {} != {}'.format(len(self._train_input_filepaths), len(self._train_output_filepaths)))
		if len(self._val_input_filepaths) != len(self._val_output_filepaths):
			raise ValueError('The lengths of validation input and output data are different: {} != {}'.format(len(self._val_input_filepaths), len(self._val_output_filepaths)))
		if len(self._test_input_filepaths) != len(self._test_output_filepaths):
			raise ValueError('The lengths of test input and output data are different: {} != {}'.format(len(self._test_input_filepaths), len(self._test_output_filepaths)))

		#--------------------
		if 'posix' == os.name:
			data_home_dir_path = '/home/sangwook/my_dataset'
		else:
			data_home_dir_path = 'D:/dataset'
		synth90k_data_dir_path = data_home_dir_path + '/pattern_recognition/language_processing/mjsynth/mnt/ramdisk/max/90kDICT32px'
		self._train_data_info, self._val_data_info, self._test_data_info = Synth90kDataGenerator._loadDataInfo(synth90k_data_dir_path)

	def getTrainBatches(self, batch_size, shuffle=True, *args, **kwargs):
		self._startAugmentationThread()
		self._generateValidationBatches()

		return self._generateBatches(self._trainFileBatchLoader, self._trainDirMgr, batch_size, shuffle, phase='train')

		#--------------------
		# FIXME [fix] >> Bad position.
		self._augmentation_worker_thread.join()

	def hasValidationData(self):
		return self.hasTestData()

	def getValidationData(self, *args, **kwargs):
		return self.getTestData(*args, **kwargs)

	def getValidationBatches(self, batch_size=None, shuffle=False, *args, **kwargs):
		self._generateValidationBatches()			

		return self._generateBatches(self._valFileBatchLoader, self._valDirMgr, batch_size, shuffle=False, phase='validation')

	def hasTestData(self):
		return self._test_inputs is not None and self._test_outputs is not None and len(self._test_inputs) > 0

	def getTestData(self, *args, **kwargs):
		return (self._test_inputs, self._test_outputs), (0 if self._test_inputs is None else len(self._test_inputs))

	def getTestBatches(self, batch_size=None, shuffle=False, *args, **kwargs):
		self._generateTestBatches()			

		return self._generateBatches(self._testFileBatchLoader, self._testDirMgr, batch_size, shuffle=False, phase='test')

	def _generateBatches(self, batchLoader, dirMgr, batch_size, shuffle=True, phase='', *args, **kwargs):
		print('\tWaiting for a {} batch directory...'.format(phase))
		while True:
			dir_path = dirMgr.requestDirectory()
			if dir_path is not None:
				break
			else:
				time.sleep(0.1)
		print('\tGot a {} batch directory: {}.'.format(phase, dir_path))

		return batchLoader.loadBatches(dir_path)  # Loads batches.

	def _startAugmentationThread(self):
		if not self._isAugmentationThreadStarted:
			# Augmentation: multithreading + multiprocessing.
			self._augmentation_worker_thread.start()
			self._isAugmentationThreadStarted = True

	def _generateValidationBatches(self):
		if self._isValidationBatchesGenerated:
			return

		print('\tWaiting for a validation batch directory...')
		while True:
			val_dir_path = self._valDirMgr.requestDirectory()
			if val_dir_path is not None:
				break
			else:
				time.sleep(0.1)
		print('\tGot a validation batch directory: {}.'.format(val_dir_path))

		is_time_major, is_output_augmented = False, False  # Don't care.
		valFileBatchGenerator = NpzFileBatchGeneratorWithNpyFileInput(self._val_input_filepaths, self._val_output_filepaths, self._num_loaded_files_at_a_time, batch_size, shuffle, is_time_major=is_time_major, augmenter=self._augmenter, is_output_augmented=is_output_augmented, batch_info_csv_filename=self._batch_info_csv_filename)
		num_saved_examples  = valFileBatchGenerator.saveBatches(val_dir_path)  # Generates and saves batches.
		print('\t#saved examples = {}.'.format(num_saved_examples))

		self._valDirMgr.returnDirectory(val_dir_path)		

		self._isValidationBatchesGenerated = True

	def _generateTestBatches(self):
		if self._isTestBatchesGenerated:
			return

		print('\tWaiting for a test batch directory...')
		while True:
			test_dir_path = self._testDirMgr.requestDirectory()
			if test_dir_path is not None:
				break
			else:
				time.sleep(0.1)
		print('\tGot a test batch directory: {}.'.format(test_dir_path))

		is_time_major, is_output_augmented = False, False  # Don't care.
		testFileBatchGenerator = NpzFileBatchGeneratorWithNpyFileInput(self._test_input_filepaths, self._test_output_filepaths, self._num_loaded_files_at_a_time, batch_size, shuffle, is_time_major=is_time_major, augmenter=self._augmenter, is_output_augmented=is_output_augmented, batch_info_csv_filename=self._batch_info_csv_filename)
		num_saved_examples = testFileBatchGenerator.saveBatches(test_dir_path)  # Generates and saves batches.
		print('\t#saved examples = {}.'.format(num_saved_examples))

		self._testDirMgr.returnDirectory(test_dir_path)				

		self._isTestBatchesGenerated = True

	@staticmethod
	def _loadDataFromNpyFiles(synth90k_base_dir_path):
		train_npy_file_csv_filepath = synth90k_base_dir_path + '/train/npy_file_info.csv'
		val_npy_file_csv_filepath = synth90k_base_dir_path + '/val/npy_file_info.csv'
		test_npy_file_csv_filepath = synth90k_base_dir_path + '/test/npy_file_info.csv'

		train_input_filepaths, train_output_filepaths, train_example_counts = swl_util.load_filepaths_from_npy_file_info(train_npy_file_csv_filepath)
		val_input_filepaths, val_output_filepaths, val_example_counts = swl_util.load_filepaths_from_npy_file_info(val_npy_file_csv_filepath)
		test_input_filepaths, test_output_filepaths, test_example_counts = swl_util.load_filepaths_from_npy_file_info(test_npy_file_csv_filepath)

		return train_input_filepaths, train_output_filepaths, val_input_filepaths, val_output_filepaths, test_input_filepaths, test_output_filepaths

	@staticmethod
	def _loadDataInfo(data_dir_path, subset_ratio=None):
		"""
		Inputs:
			data_dir_path (string): The directory path of Synth90k dataset.
			subset_ratio (float or None): The ratio of subset of data. 0.0 < subset_ratio <= 1.0.
		"""

		import swl.language_processing.synth90k_dataset as synth90k_dataset

		# filepath (filename: index_text_lexicon-idx) lexicon-idx.
		all_anno_filepath = data_dir_path + '/annotation.txt'  # 8,919,273 files.
		train_anno_filepath = data_dir_path + '/annotation_train.txt'  # 7,224,612 files.
		val_anno_filepath = data_dir_path + '/annotation_val.txt'  # 802,734 files.
		test_anno_filepath = data_dir_path + '/annotation_test.txt'  # 891,927 files.
		lexicon_filepath = data_dir_path + '/lexicon.txt'  # 88,172 words.

		print('Start loading lexicon...')
		start_time = time.time()
		lexicon = synth90k_dataset.load_synth90k_lexicon(lexicon_filepath)
		print('\tLexicon size =', len(lexicon))
		print('End loading lexicon: {} secs.'.format(time.time() - start_time))

		#--------------------
		print('Start loading train data info...')
		start_time = time.time()
		train_data_info = synth90k_dataset.load_synth90k_data_info(train_anno_filepath, data_dir_path, lexicon, subset_ratio)
		print('\tTrain data size =', len(train_data_info))
		print('End loading train data info: {} secs.'.format(time.time() - start_time))

		print('Start loading validation data info...')
		start_time = time.time()
		val_data_info = synth90k_dataset.load_synth90k_data_info(val_anno_filepath, data_dir_path, lexicon, subset_ratio)
		print('\tValiation data size =', len(val_data_info))
		print('End loading validation data info: {} secs.'.format(time.time() - start_time))

		print('Start loading test data info...')
		start_time = time.time()
		test_data_info = synth90k_dataset.load_synth90k_data_info(test_anno_filepath, data_dir_path, lexicon, subset_ratio)
		print('\tTest data size =', len(test_data))
		print('End loading test data info: {} secs.'.format(time.time() - start_time))

		return train_data_info, val_data_info, test_data_info

	@staticmethod
	def augmentation_worker_thread_proc(num_processes, manager, lock, train_batch_dir_path_prefix, num_train_batch_dirs, augmenter, train_input_filepaths, train_output_filepaths, num_loaded_files_at_a_time, batch_size, shuffle, batch_info_csv_filename):
		trainDirMgr_mp = manager.TwoStepWorkingDirectoryManager(train_batch_dir_path_prefix, num_train_batch_dirs)
		#valDirMgr_mp = manager.WorkingDirectoryManager(val_batch_dir_path_prefix, num_val_batch_dirs)

		#trainFileBatchGenerator_mp = manager.NpzFileBatchGeneratorWithNpyFileInput(train_input_filepaths, train_output_filepaths, num_loaded_files_at_a_time, batch_size, shuffle, False, augmenter=augmenter, is_output_augmented=is_output_augmented, batch_info_csv_filename=batch_info_csv_filename)
		#trainFileBatchLoader_mp = manager.NpzFileBatchLoader(batch_info_csv_filename, data_processing_functor=Synth90kPreprocessor(is_sparse_output))
		#valFileBatchLoader_mp = manager.NpzFileBatchLoader(batch_info_csv_filename, data_processing_functor=Synth90kPreprocessor(is_sparse_output))

		#timeout = 10
		timeout = None
		with mp.Pool(processes=num_processes, initializer=Synth90kDataGenerator.initialize_lock, initargs=(lock,)) as pool:
			is_output_augmented, is_time_major = False, False  # Don't care.
			data_augmentation_results = pool.map_async(partial(Synth90kDataGenerator.augmentation_worker_process_proc, augmenter, is_output_augmented, trainDirMgr_mp, train_input_filepaths, train_output_filepaths, num_loaded_files_at_a_time, batch_size, shuffle, is_time_major, batch_info_csv_filename), [epoch for epoch in range(num_epochs)])

			data_augmentation_results.get(timeout)

	@staticmethod
	def initialize_lock(lock):
		global global_lock
		global_lock = lock

	# REF [function] >> augmentation_worker_proc() in ${SWL_PYTHON_HOME}/python/test/machine_learning/batch_generator_and_loader_test.py.
	@staticmethod
	def augmentation_worker_process_proc(augmenter, is_output_augmented, dirMgr, input_filepaths, output_filepaths, num_loaded_files_at_a_time, batch_size, shuffle, is_time_major, batch_info_csv_filename, epoch):
		print('\t{}: Start augmentation worker process: epoch #{}.'.format(os.getpid(), epoch))
		print('\t{}: Request a preparatory train directory.'.format(os.getpid()))
		while True:
			with global_lock:
				dir_path = dirMgr.requestDirectory(is_workable=False)

			if dir_path is not None:
				break
			else:
				time.sleep(0.1)
		print('\t{}: Got a preparatory train directory: {}.'.format(os.getpid(), dir_path))

		#--------------------
		fileBatchGenerator = NpzFileBatchGeneratorWithNpyFileInput(input_filepaths, output_filepaths, num_loaded_files_at_a_time, batch_size, shuffle, is_time_major, augmenter=augmenter, is_output_augmented=is_output_augmented, batch_info_csv_filename=batch_info_csv_filename)
		num_saved_examples = fileBatchGenerator.saveBatches(dir_path)  # Generates and saves batches.
		print('\t{}: #saved examples = {}.'.format(os.getpid(), num_saved_examples))

		#--------------------
		with global_lock:
			dirMgr.returnDirectory(dir_path)
		print('\t{}: Returned a directory: {}.'.format(os.getpid(), dir_path))
		print('\t{}: End augmentation worker process.'.format(os.getpid()))