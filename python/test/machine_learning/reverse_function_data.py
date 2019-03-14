import numpy as np
import tensorflow as tf
from random import choice, randrange
from swl.machine_learning.data_generator import DataGenerator

#%%------------------------------------------------------------------
# REF [site] >> https://talbaumel.github.io/attention/

class ReverseFunctionDataset(object):
	def __init__(self):
		characters = 'abcd'
		self._SOS = '<SOS>'  # All strings will start with the Start-Of-String token.
		self._EOS = '<EOS>'  # All strings will end with the End-Of-String token.
		self._characters = [self._SOS] + list(characters) + [self._EOS]

		self._VOCAB_SIZE = len(self._characters)
		self._MAX_STRING_LEN = 15
		#self._MAX_TOKEN_LEN = self._MAX_STRING_LEN
		#self._MAX_TOKEN_LEN = self._MAX_STRING_LEN + 1
		self._MAX_TOKEN_LEN = self._MAX_STRING_LEN + 2

		self._label_int2char = list(self._characters)
		self._label_char2int = {c:i for i, c in enumerate(self._characters)}

		self._num_train_data = 3000
		self._num_val_data = 100

		#print(self._sample_model(4, 5))
		#print(self._sample_model(5, 10))
	
	@property
	def vocab_size(self):
		return self._VOCAB_SIZE

	@property
	def max_token_len(self):
		return self._MAX_TOKEN_LEN

	@property
	def start_token(self):
		return self._label_char2int[self._SOS]

	@property
	def end_token(self):
		return self._label_char2int[self._EOS]

	def generate_dataset(self, is_time_major):
		train_string_list = self._create_string_dataset(self._num_train_data, self._MAX_STRING_LEN)
		val_string_list = self._create_string_dataset(self._num_val_data, self._MAX_STRING_LEN)

		train_numeric_list = self._convert_string_dataset_to_numeric_dataset(train_string_list)
		val_numeric_list = self._convert_string_dataset_to_numeric_dataset(val_string_list)

		if True:
			# Uses a fixed-length dataset of type np.array.

			train_input_seqs, train_output_seqs, train_output_seqs_ahead_of_one_timestep = self._create_array_dataset(train_numeric_list, self._MAX_TOKEN_LEN, self._VOCAB_SIZE, is_time_major)
			#val_input_seqs, _, val_output_seqs_ahead_of_one_timestep = self._create_array_dataset(val_numeric_list, self._MAX_TOKEN_LEN, is_time_major)
			val_input_seqs, val_output_seqs, val_output_seqs_ahead_of_one_timestep = self._create_array_dataset(val_numeric_list, self._MAX_TOKEN_LEN, self._VOCAB_SIZE, is_time_major)
		else:
			# Uses a variable-length dataset of a list of np.array.
			# TensorFlow internally uses np.arary for tf.placeholder. (?)

			train_input_seqs, train_output_seqs, train_output_seqs_ahead_of_one_timestep = self._create_list_dataset(train_numeric_list, self._VOCAB_SIZE, is_time_major)
			#val_input_seqs, _, val_output_seqs_ahead_of_one_timestep = self._create_list_dataset(val_numeric_list, is_time_major)
			val_input_seqs, val_output_seqs, val_output_seqs_ahead_of_one_timestep = self._create_list_dataset(val_numeric_list, self._VOCAB_SIZE, is_time_major)

		return train_input_seqs, train_output_seqs, train_output_seqs_ahead_of_one_timestep, val_input_seqs, val_output_seqs, val_output_seqs_ahead_of_one_timestep

	# Character strings -> numeric data.
	def to_numeric_data(self, char_strs):
		num_data = np.full((len(char_strs), self._MAX_TOKEN_LEN), self._label_char2int[self._EOS])
		for (i, str) in enumerate(char_strs):
			tmp = np.array(self._str2datum(str))
			num_data[i,:tmp.shape[0]] = tmp
		num_data.reshape((-1,) + num_data.shape)
		return tf.keras.utils.to_categorical(num_data, self._VOCAB_SIZE).reshape(num_data.shape + (-1,))

	# Numeric data -> character strings.
	def to_char_strings(self, num_data, has_start_token=True):
		num_data = np.argmax(num_data, axis=-1)
		char_strs = []
		for dat in num_data:
			char_strs.append(self._datum2str(dat, has_start_token))
		return char_strs

	def _sample_model(self, min_length, max_length):
		random_length = randrange(min_length, max_length)
		# Pick a random length.
		random_char_list = [choice(self._characters[1:-1]) for _ in range(random_length)]
		# Pick random chars.
		random_str = ''.join(random_char_list)
		return random_str, random_str[::-1]  # Return the random string and its reverse.

	# A character string to a numeric datum(numeric list).
	def _str2datum(self, str):
		#str = list(str) + [self._EOS]
		str = [self._SOS] + list(str) + [self._EOS]
		return [self._label_char2int[ch] for ch in str]

	# A numeric datum(numeric list) to a character string.
	def _datum2str(self, datum, has_start_token):
		locs = np.where(self._label_char2int[self._EOS] == datum)
		datum = datum[:locs[0][0]]
		if has_start_token:
			return ''.join([self._label_int2char[no] for no in datum[1:]])
		else:
			return ''.join([self._label_int2char[no] for no in datum[:]])

	# Preprocessing function for character strings.
	def _preprocess_string(self, str):
		return self._str2datum(str)

	def _create_dataset(self, dataset, window_size=1):
		dataX, dataY = [], []
		# FIXME [check] >> Which one is correct?
		#for i in range(len(dataset) - window_size - 1):
		for i in range(len(dataset) - window_size):
			dataX.append(dataset[i:(i + window_size)])
			dataY.append(dataset[i + window_size])  # Next character.
		return np.array(dataX), np.array(dataY)

	def _create_string_dataset(self, num_data, str_len):
		return [self._sample_model(1, str_len) for _ in range(num_data)]

	def _convert_string_dataset_to_numeric_dataset(self, dataset):
		data = []
		for input_str, output_str in dataset:
			#_, x = self._create_dataset(self._str2datum(input_str), window_size=0)
			#_, y = self._create_dataset(self._str2datum(output_str), window_size=0)
			x = self._str2datum(input_str)
			y = self._str2datum(output_str)
			data.append((x, y))
		return data

	def _max_len(self, dataset):
		num_data = len(dataset)
		ml = 0
		for i in range(num_data):
			if len(dataset[i][0]) > ml:
				ml = len(dataset[i][0])
		return ml

	# Fixed-length dataset.
	def _create_array_dataset(self, input_output_pairs, max_time_steps, num_features, is_time_major):
		num_samples = len(input_output_pairs)
		input_data = np.full((num_samples, max_time_steps), self._label_char2int[self._EOS])
		output_data = np.full((num_samples, max_time_steps), self._label_char2int[self._EOS])
		output_data_ahead_of_one_timestep = np.full((num_samples, max_time_steps), self._label_char2int[self._EOS])
		for (i, (inp, outp)) in enumerate(input_output_pairs):
			input_data[i,:len(inp)] = np.array(inp)
			outa = np.array(outp)
			output_data[i,:len(outp)] = outa
			output_data_ahead_of_one_timestep[i,:(len(outp) - 1)] = outa[1:]

		# (samples, time-steps) -> (samples, time-steps, features).
		input_data = tf.keras.utils.to_categorical(input_data, num_features).reshape(input_data.shape + (-1,))
		output_data = tf.keras.utils.to_categorical(output_data, num_features).reshape(output_data.shape + (-1,))
		output_data_ahead_of_one_timestep = tf.keras.utils.to_categorical(output_data_ahead_of_one_timestep, num_features).reshape(output_data_ahead_of_one_timestep.shape + (-1,))

		if is_time_major:
			# (time-steps, samples, features) -> (samples, time-steps, features).
			return np.stack(input_data, axis=1), np.stack(output_data, axis=1), np.stack(output_data_ahead_of_one_timestep, axis=1)
		else:
			return input_data, output_data, output_data_ahead_of_one_timestep

	# Variable-length dataset.
	def _create_list_dataset(self, input_output_pairs, num_features, is_time_major):
		input_data, output_data, output_data_ahead_of_one_timestep = [], [], []
		if is_time_major:
			# Cannot create a time-major dataset.
			raise NotImplementedError
		else:
			for (inp, outp) in input_output_pairs:
				input_data.append(np.array(inp))
				output_data.append(np.array(outp))
				output_data_ahead_of_one_timestep.append(np.array(outp[1:]))

			# A 'samples' list of (time-steps) -> A 'samples' list of (time-steps, features).
			tmp_data, tmp_labels, tmp_labels_ahead = [], [], []
			for (dat, lbl, lbl_ahead) in zip(input_data, output_data, output_data_ahead_of_one_timestep):
				tmp_data.append(tf.keras.utils.to_categorical(dat, num_features).reshape(dat.shape + (-1,)))
				tmp_labels.append(tf.keras.utils.to_categorical(lbl, num_features).reshape(lbl.shape + (-1,)))
				tmp_labels_ahead.append(tf.keras.utils.to_categorical(lbl_ahead, num_features).reshape(lbl_ahead.shape + (-1,)))
			input_data, output_data, output_data_ahead_of_one_timestep = tmp_data, tmp_labels, tmp_labels_ahead
		return input_data, output_data, output_data_ahead_of_one_timestep

	def _decode_predicted_sequence(self, prediction):
		num_tokens = prediction.shape[1]
		predicted_sentence = ''
		for i in range(num_tokens):
			token_index = np.argmax(prediction[0, i, :])
			ch = self._label_int2char[token_index]
			predicted_sentence += ch

		return predicted_sentence;

	def _decode_sequence(self, encoder_model, decoder_model, input_seq):
		# Encode the input as state vectors.
		states_output = encoder_model.predict(input_seq)

		# Generate empty target sequence of length 1.
		target_seq = np.zeros((1, 1, self._VOCAB_SIZE))
		# Populate the first character of target sequence with the start character.
		target_seq[0, 0, 0] = 1  # <SOS>.

		# Sampling loop for a batch of sequences (to simplify, here we assume a batch of size 1).
		stop_condition = False
		decoded_sentence = ''
		while not stop_condition:
			output_tokens, h, c = decoder_model.predict([target_seq] + states_output)

			# Sample a token.
			sampled_token_index = np.argmax(output_tokens[0, -1, :])
			sampled_char = self._label_int2char[sampled_token_index]
			decoded_sentence += sampled_char

			# Exit condition: either hit max length or find stop character.
			if (sampled_char == self._EOS or len(decoded_sentence) > self._MAX_TOKEN_LEN):
				stop_condition = True

			# Update the target sequence (of length 1).
			target_seq = np.zeros((1, 1, self._VOCAB_SIZE))
			target_seq[0, 0, sampled_token_index] = 1

			# Update states.
			states_output = [h, c]

		return decoded_sentence

#%%------------------------------------------------------------------
# ReverseFunctionDataGenerator.

#class ReverseFunctionDataGenerator(abc.ABC):
class ReverseFunctionDataGenerator(DataGenerator):
	def __init__(self, is_time_major, is_dynamic):
		super().__init__()

		self._is_time_major = is_time_major
		self._is_dynamic = is_dynamic
		self._dataset = ReverseFunctionDataset()

		self._train_encoder_inputs, self._train_decoder_inputs, self._train_decoder_outputs, self._val_encoder_inputs, self._val_decoder_inputs, self._val_decoder_outputs = (None,) * 6

	@property
	def dataset(self):
		if self._dataset is None:
			raise TypeError
		return self._dataset

	def initialize(self):
		# NOTICE [info] >> How to use the hidden state c of an encoder in a decoder?
		#	1) The hidden state c of the encoder is used as the initial state of the decoder and the previous output of the decoder may be used as its only input.
		#	2) The previous output of the decoder is used as its input along with the hidden state c of the encoder.
		self._train_encoder_inputs, self._train_decoder_inputs, self._train_decoder_outputs, self._val_encoder_inputs, self._val_decoder_inputs, self._val_decoder_outputs = self._dataset.generate_dataset(self._is_time_major)

		if self._train_encoder_inputs is None or self._train_decoder_inputs is None or self._train_decoder_outputs is None:
			raise ValueError('Train inputs or outputs is None')
		if len(self._train_encoder_inputs) != len(self._train_decoder_inputs) or len(self._train_encoder_inputs) != len(self._train_decoder_outputs):
			raise ValueError('The lengths of train inputs and outputs are different: {}, {}, {}'.format(len(self._train_encoder_inputs), len(self._train_decoder_inputs), len(self._train_decoder_outputs)))
		if self._val_encoder_inputs is None or self._val_decoder_inputs is None or self._val_decoder_outputs is None:
			raise ValueError('Test inputs or outputs is None')
		if len(self._val_encoder_inputs) != len(self._val_decoder_inputs) or len(self._val_encoder_inputs) != len(self._val_decoder_outputs):
			raise ValueError('The lengths of test inputs and outputs are different: {}, {}, {}'.format(len(self._val_encoder_inputs), len(self._val_decoder_inputs), len(self._val_decoder_outputs)))

	def getShapes(self):
		if self._is_dynamic:
			# Dynamic RNNs use variable-length dataset.
			# TODO [improve] >> Training & validation datasets are still fixed-length (static).
			encoder_input_shape = (None, None, self._dataset.vocab_size)
			decoder_input_shape = (None, None, self._dataset.vocab_size)
			decoder_output_shape = (None, None, self._dataset.vocab_size)
		else:
			# Static RNNs use fixed-length dataset.
			if self._is_time_major:
				# (time-steps, samples, features).
				encoder_input_shape = (self._dataset.max_token_len, None, self._dataset.vocab_size)
				decoder_input_shape = (self._dataset.max_token_len, None, self._dataset.vocab_size)
				decoder_output_shape = (self._dataset.max_token_len, None, self._dataset.vocab_size)
			else:
				# (samples, time-steps, features).
				encoder_input_shape = (None, self._dataset.max_token_len, self._dataset.vocab_size)
				decoder_input_shape = (None, self._dataset.max_token_len, self._dataset.vocab_size)
				decoder_output_shape = (None, self._dataset.max_token_len, self._dataset.vocab_size)

		return encoder_input_shape, decoder_input_shape, decoder_output_shape

	def getTrainBatches(self, batch_size, shuffle=True, *args, **kwargs):
		if self._train_encoder_inputs is None or self._train_decoder_inputs is None  or self._train_decoder_outputs is None:
			raise ValueError('At least one of train input or output data is None')

		return self._generateBatches(self._train_encoder_inputs, self._train_decoder_inputs, self._train_decoder_outputs, batch_size, shuffle)

	def hasValidationData(self):
		return self._val_encoder_inputs is not None and self._val_decoder_inputs is not None and self._val_decoder_outputs is not None and len(self._val_encoder_inputs) > 0

	def getValidationData(self, *args, **kwargs):
		return (self._val_encoder_inputs, self._val_decoder_inputs, self._val_decoder_outputs), (0 if self._val_encoder_inputs is None else len(self._val_encoder_inputs))

	def getValidationBatches(self, batch_size=None, shuffle=False, *args, **kwargs):
		if self._val_encoder_inputs is None or self._val_decoder_inputs is None or self._val_decoder_outputs is None:
			raise ValueError('At least one of validation input or output data is None')

		return self._generateBatches(self._val_encoder_inputs, self._val_decoder_inputs, self._val_decoder_outputs, batch_size, shuffle=False)

	def hasTestData(self):
		return False

	def getTestData(self, *args, **kwargs):
		raise NotImplementedError

	def getTestBatches(self, batch_size=None, shuffle=False, *args, **kwargs):
		raise NotImplementedError

	def _generateBatches(self, encoder_inputs, decoder_inputs, decoder_outputs, batch_size, shuffle=True, *args, **kwargs):
		num_examples = len(encoder_inputs)
		if batch_size is None:
			batch_size = num_examples
		if batch_size <= 0:
			raise ValueError('Invalid batch size: {}'.format(batch_size))

		indices = np.arange(num_examples)
		if shuffle:
			np.random.shuffle(indices)

		start_idx = 0
		while True:
			end_idx = start_idx + batch_size
			batch_indices = indices[start_idx:end_idx]
			if batch_indices.size > 0:  # If batch_indices is non-empty.
				# FIXME [fix] >> Does not work correctly in time-major data.
				batch_enc_inputs, batch_dec_inputs, batch_dec_outputs = encoder_inputs[batch_indices], decoder_inputs[batch_indices], decoder_outputs[batch_indices]
				if batch_enc_inputs.size > 0 and batch_dec_inputs.size > 0 and batch_dec_outputs.size > 0:  # If batch_enc_inputs, batch_dec_inputs, and batch_dec_outputs are non-empty.
					yield (batch_enc_inputs, batch_dec_inputs, batch_dec_outputs), batch_indices.size

			if end_idx >= num_examples:
				break
			start_idx = end_idx