# Path to libcudnn.so.
#export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH

#--------------------
import os, sys
if 'posix' == os.name:
	swl_python_home_dir_path = '/home/sangwook/work/SWL_github/python'
	lib_home_dir_path = '/home/sangwook/lib_repo/python'
else:
	swl_python_home_dir_path = 'D:/work/SWL_github/python'
	lib_home_dir_path = 'D:/lib_repo/python'
	#lib_home_dir_path = 'D:/lib_repo/python/rnd'
#sys.path.append('../../../src')
sys.path.append(swl_python_home_dir_path + '/src')

#os.chdir(swl_python_home_dir_path + '/test/machine_learning/tensorflow')

#--------------------
import numpy as np
import tensorflow as tf
from simple_seq2seq_encdec import SimpleSeq2SeqEncoderDecoder
from simple_seq2seq_encdec_tf_attention import SimpleSeq2SeqEncoderDecoderWithTfAttention
from simple_neural_net_trainer import SimpleNeuralNetGradientTrainer
from swl.machine_learning.tensorflow.neural_net_evaluator import NeuralNetEvaluator
from swl.machine_learning.tensorflow.neural_net_inferrer import NeuralNetInferrer
import swl.machine_learning.util as swl_ml_util
from rda_plant_util import RdaPlantDataset
import time

#%%------------------------------------------------------------------

def train_neural_net(session, nnTrainer, train_encoder_input_seqs, train_decoder_input_seqs, train_decoder_output_seqs, val_encoder_input_seqs, val_decoder_input_seqs, val_decoder_output_seqs, batch_size, num_epochs, shuffle, does_resume_training, saver, output_dir_path, model_dir_path, train_summary_dir_path, val_summary_dir_path):
	if does_resume_training:
		print('[SWL] Info: Resume training...')

		# Load a model.
		ckpt = tf.train.get_checkpoint_state(model_dir_path)
		saver.restore(session, ckpt.model_checkpoint_path)
		#saver.restore(session, tf.train.latest_checkpoint(model_dir_path))
		print('[SWL] Info: Restored a model.')
	else:
		print('[SWL] Info: Start training...')

	start_time = time.time()
	history = nnTrainer.train_seq2seq(session, train_encoder_input_seqs, train_decoder_input_seqs, train_decoder_output_seqs, val_encoder_input_seqs, val_decoder_input_seqs, val_decoder_output_seqs, batch_size, num_epochs, shuffle, saver=saver, model_save_dir_path=model_dir_path, train_summary_dir_path=train_summary_dir_path, val_summary_dir_path=val_summary_dir_path)
	print('\tTraining time = {}'.format(time.time() - start_time))

	# Display results.
	#swl_ml_util.display_train_history(history)
	if output_dir_path is not None:
		swl_ml_util.save_train_history(history, output_dir_path)
	print('[SWL] Info: End training...')

def evaluate_neural_net(session, nnEvaluator, val_encoder_input_seqs, val_decoder_input_seqs, val_decoder_output_seqs, batch_size, saver=None, model_dir_path=None):
	if saver is not None and model_dir_path is not None:
		# Load a model.
		ckpt = tf.train.get_checkpoint_state(model_dir_path)
		saver.restore(session, ckpt.model_checkpoint_path)
		#saver.restore(session, tf.train.latest_checkpoint(model_dir_path))
		print('[SWL] Info: Loaded a model.')

	print('[SWL] Info: Start evaluation...')
	start_time = time.time()
	val_loss, val_acc = nnEvaluator.evaluate_seq2seq(session, val_encoder_input_seqs, val_decoder_input_seqs, val_decoder_output_seqs, batch_size)
	print('\tEvaluation time = {}'.format(time.time() - start_time))
	print('\tTest loss = {}, test accurary = {}'.format(val_loss, val_acc))
	print('[SWL] Info: End evaluation...')

def infer_by_neural_net(session, nnInferrer, dataset, test_strs, batch_size, saver=None, model_dir_path=None):
	# Character strings -> numeric data.
	test_data = dataset.to_numeric_data(test_strs)

	if saver is not None and model_dir_path is not None:
		# Load a model.
		ckpt = tf.train.get_checkpoint_state(model_dir_path)
		saver.restore(session, ckpt.model_checkpoint_path)
		#saver.restore(session, tf.train.latest_checkpoint(model_dir_path))
		print('[SWL] Info: Loaded a model.')

	print('[SWL] Info: Start inferring...')
	start_time = time.time()
	inferences = nnInferrer.infer(session, test_data, batch_size)
	print('\tInference time = {}'.format(time.time() - start_time))

	# Numeric data -> character strings.
	inferred_strs = dataset.to_char_strings(inferences, has_start_token=False)

	print('\tTest strings = {}, inferred strings = {}'.format(test_strs, inferred_strs))
	print('[SWL] Info: End inferring...')

#%%------------------------------------------------------------------

import datetime

def make_dir(dir_path):
	if not os.path.exists(dir_path):
		try:
			os.makedirs(dir_path)
		except OSError as ex:
			if os.errno.EEXIST != ex.errno:
				raise

def pad_image(img, target_height, target_width):
	if 2 == img.ndim:
		height, width = img.shape
	elif 3 == img.ndim:
		height, width, _ = img.shape
	else:
		assert 2 == img.ndim or 3 == img.ndim, 'The dimension of an image is not proper.'

	left_margin = (target_width - width) // 2
	right_margin = target_width - width - left_margin
	top_margin = (target_height - height) // 2
	bottom_margin = target_height - height - top_margin
	if 2 == img.ndim:
		return np.pad(img, ((bottom_margin, top_margin), (left_margin, right_margin)), 'edge')
	else:
		return np.pad(img, ((bottom_margin, top_margin), (left_margin, right_margin), (0, 0)), 'edge')

def create_seq2seq_encoder_decoder(encoder_input_shape, decoder_input_shape, decoder_output_shape, dataset, is_time_major):
	# Sequence-to-sequence encoder-decoder model w/o attention.
	return SimpleSeq2SeqEncoderDecoder(encoder_input_shape, decoder_input_shape, decoder_output_shape, dataset.start_token, dataset.end_token, is_time_major=is_time_major)

def main():
	#np.random.seed(7)

	does_need_training = True
	does_resume_training = False

	#--------------------
	# Prepare directories.

	output_dir_prefix = 'reverse_function_seq2seq'
	output_dir_suffix = datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
	#output_dir_suffix = '20180222T144236'

	output_dir_path = './{}_{}'.format(output_dir_prefix, output_dir_suffix)
	model_dir_path = '{}/model'.format(output_dir_path)
	inference_dir_path = '{}/inference'.format(output_dir_path)
	train_summary_dir_path = '{}/train_log'.format(output_dir_path)
	val_summary_dir_path = '{}/val_log'.format(output_dir_path)

	make_dir(model_dir_path)
	make_dir(inference_dir_path)
	make_dir(train_summary_dir_path)
	make_dir(val_summary_dir_path)

	#--------------------
	# Prepare data.

	if 'posix' == os.name:
		#data_home_dir_path = '/home/sangwook/my_dataset'
		data_home_dir_path = '/home/HDD1/sangwook/my_dataset'
	else:
		data_home_dir_path = 'D:/dataset'
	data_dir_path = data_home_dir_path + '/phenotyping/RDA/all_plants_mask'
	plant_mask_list_file_name = '/plant_mask_list.json'

	plant_mask_list, max_size = RdaPlantDataset.load_masks_from_json(data_dir_path, plant_mask_list_file_name)
	#plant: plant_mask_list[*][0]
	#masks: plant_mask_list[*][1][0] ~ plant_mask_list[*][1][n]
	max_len = max(max_size)
	for pm_pair in plant_mask_list:
		pm_pair[0] = pad_image(pm_pair[0], max_len, max_len)
		for (idx, mask) in enumerate(pm_pair[1]):
			#mask = pad_image(mask, max_len, max_len)  # Not correctly working.
			pm_pair[1][idx] = pad_image(mask, max_len, max_len)

	#--------------------
	# Create models, sessions, and graphs.

	batch_size = 4  # Number of samples per gradient update.
	num_epochs = 70  # Number of times to iterate over training data.
	shuffle = True

	# Create graphs.
	if does_need_training:
		train_graph = tf.Graph()
	eval_graph = tf.Graph()
	infer_graph = tf.Graph()

	if does_need_training:
		with train_graph.as_default():
			# Create a model.
			rnnModelForTraining = create_seq2seq_encoder_decoder(encoder_input_shape, decoder_input_shape, decoder_output_shape, dataset, is_time_major)
			rnnModelForTraining.create_training_model()

			# Create a trainer.
			initial_epoch = 0
			#nnTrainer = SimpleNeuralNetTrainer(rnnModelForTraining, initial_epoch)
			nnTrainer = SimpleNeuralNetGradientTrainer(rnnModelForTraining, initial_epoch)

			# Create a saver.
			#	Save a model every 2 hours and maximum 5 latest models are saved.
			train_saver = tf.train.Saver(max_to_keep=5, keep_checkpoint_every_n_hours=2)

			initializer = tf.global_variables_initializer()

	with eval_graph.as_default():
		# Create a model.
		rnnModelForEvaluation = create_seq2seq_encoder_decoder(encoder_input_shape, decoder_input_shape, decoder_output_shape, dataset, is_time_major)
		rnnModelForEvaluation.create_evaluation_model()

		# Create an evaluator.
		nnEvaluator = NeuralNetEvaluator(rnnModelForEvaluation)

		# Create a saver.
		eval_saver = tf.train.Saver()

	with infer_graph.as_default():
		# Create a model.
		rnnModelForInference = create_seq2seq_encoder_decoder(encoder_input_shape, decoder_input_shape, decoder_output_shape, dataset, is_time_major)
		rnnModelForInference.create_inference_model()

		# Create an inferrer.
		nnInferrer = NeuralNetInferrer(rnnModelForInference)

		# Create a saver.
		infer_saver = tf.train.Saver()

	# Create sessions.
	config = tf.ConfigProto()
	#config.allow_soft_placement = True
	config.log_device_placement = True
	config.gpu_options.allow_growth = True
	#config.gpu_options.per_process_gpu_memory_fraction = 0.4  # Only allocate 40% of the total memory of each GPU.

	if does_need_training:
		train_session = tf.Session(graph=train_graph, config=config)
	eval_session = tf.Session(graph=eval_graph, config=config)
	infer_session = tf.Session(graph=infer_graph, config=config)

	# Initialize.
	if does_need_training:
		train_session.run(initializer)

	#%%------------------------------------------------------------------
	# Train.

	if does_need_training:
		total_elapsed_time = time.time()
		with train_session.as_default() as sess:
			with sess.graph.as_default():
				train_neural_net(sess, nnTrainer, train_encoder_input_seqs, train_decoder_input_seqs, train_decoder_output_seqs, val_encoder_input_seqs, val_decoder_input_seqs, val_decoder_output_seqs, batch_size, num_epochs, shuffle, does_resume_training, train_saver, output_dir_path, model_dir_path, train_summary_dir_path, val_summary_dir_path)
		print('\tTotal training time = {}'.format(time.time() - total_elapsed_time))

	#%%------------------------------------------------------------------
	# Evaluate and infer.

	total_elapsed_time = time.time()
	with eval_session.as_default() as sess:
		with sess.graph.as_default():
			evaluate_neural_net(sess, nnEvaluator, val_encoder_input_seqs, val_decoder_input_seqs, val_decoder_output_seqs, batch_size, eval_saver, model_dir_path)
	print('\tTotal evaluation time = {}'.format(time.time() - total_elapsed_time))

	total_elapsed_time = time.time()
	with infer_session.as_default() as sess:
		with sess.graph.as_default():
			test_strs = ['abc', 'cba', 'dcb', 'abcd', 'dcba', 'cdacbd', 'bcdaabccdb']
			infer_by_neural_net(sess, nnInferrer, dataset, test_strs, batch_size, infer_saver, model_dir_path)
	print('\tTotal inference time = {}'.format(time.time() - total_elapsed_time))

	#--------------------
	# Close sessions.

	if does_need_training:
		train_session.close()
		train_session = None
	eval_session.close()
	eval_session = None
	infer_session.close()
	infer_session = None

#%%------------------------------------------------------------------

if '__main__' == __name__:
	main()