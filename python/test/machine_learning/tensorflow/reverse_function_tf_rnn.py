import tensorflow as tf
from reverse_function_rnn import ReverseFunctionRNN

#%%------------------------------------------------------------------

class ReverseFunctionTensorFlowRNN(ReverseFunctionRNN):
	def __init__(self, input_shape, output_shape, is_dynamic=True, is_bidirectional=True, is_stacked=True):
		self._is_dynamic = is_dynamic
		self._is_bidirectional = is_bidirectional
		self._is_stacked = is_stacked
		super().__init__(input_shape, output_shape)

	def _create_model(self, input_tensor, is_training_tensor, input_shape, output_shape):
		with tf.variable_scope('reverse_function_tf_rnn', reuse=tf.AUTO_REUSE):
			if self._is_dynamic:
				num_classes = output_shape[-1]
				if self._is_bidirectional:
					if self._is_stacked:
						return self._create_dynamic_stacked_birnn(input_tensor, is_training_tensor, num_classes)
					else:
						return self._create_dynamic_birnn(input_tensor, is_training_tensor, num_classes)
				else:
					if self._is_stacked:
						return self._create_dynamic_stacked_rnn(input_tensor, is_training_tensor, num_classes)
					else:
						return self._create_dynamic_rnn(input_tensor, is_training_tensor, num_classes)
			else:
				num_time_steps, num_classes = input_shape[0], output_shape[-1]
				if self._is_bidirectional:
					if self._is_stacked:
						return self._create_static_stacked_birnn(input_tensor, is_training_tensor, num_time_steps, num_classes)
					else:
						return self._create_static_birnn(input_tensor, is_training_tensor, num_time_steps, num_classes)
				else:
					if self._is_stacked:
						return self._create_static_stacked_rnn(input_tensor, is_training_tensor, num_time_steps, num_classes)
					else:
						return self._create_static_rnn(input_tensor, is_training_tensor, num_time_steps, num_classes)

	def _create_dynamic_rnn(self, input_tensor, is_training_tensor, num_classes):
		"""
		num_hidden_units = 256
		keep_prob = 1.0
		"""
		num_hidden_units = 512
		keep_prob = 0.5

		# Defines a cell.
		cell = self._create_cell(num_hidden_units)
		cell = tf.contrib.rnn.DropoutWrapper(cell, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Gets cell outputs.
		#cell_outputs, cell_state = tf.nn.dynamic_rnn(cell, input_tensor, dtype=tf.float32)
		cell_outputs, _ = tf.nn.dynamic_rnn(cell, input_tensor, dtype=tf.float32)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_dynamic_stacked_rnn(self, input_tensor, is_training_tensor, num_classes):
		num_layers = 2
		"""
		num_hidden_units = 128
		keep_prob = 1.0
		"""
		num_hidden_units = 256
		keep_prob = 0.5

		# Defines cells.
		# REF [site] >> https://www.tensorflow.org/tutorials/recurrent
		stacked_cell = tf.contrib.rnn.MultiRNNCell([self._create_cell(num_hidden_units) for _ in range(num_layers)])
		stacked_cell = tf.contrib.rnn.DropoutWrapper(stacked_cell, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Gets cell outputs.
		#cell_outputs, cell_state = tf.nn.dynamic_rnn(stacked_cell, input_tensor, dtype=tf.float32)
		cell_outputs, _ = tf.nn.dynamic_rnn(stacked_cell, input_tensor, dtype=tf.float32)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_dynamic_birnn(self, input_tensor, is_training_tensor, num_classes):
		"""
		num_hidden_units = 128
		keep_prob = 1.0
		"""
		num_hidden_units = 256
		keep_prob = 0.5

		# Defines cells.
		cell_fw = self._create_cell(num_hidden_units)  # Forward cell.
		cell_fw = tf.contrib.rnn.DropoutWrapper(cell_fw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)
		cell_bw = self._create_cell(num_hidden_units)  # Backward cell.
		cell_bw = tf.contrib.rnn.DropoutWrapper(cell_bw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Gets cell outputs.
		#cell_outputs, cell_states = tf.nn.bidirectional_dynamic_rnn(cell_fw, cell_bw, input_tensor, dtype=tf.float32)
		cell_outputs, _ = tf.nn.bidirectional_dynamic_rnn(cell_fw, cell_bw, input_tensor, dtype=tf.float32)
		cell_outputs = tf.concat(cell_outputs, 2)
		#cell_states = tf.concat(cell_states, 2)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_dynamic_stacked_birnn(self, input_tensor, is_training_tensor, num_classes):
		num_layers = 2
		"""
		num_hidden_units = 64
		keep_prob = 1.0
		"""
		num_hidden_units = 128
		keep_prob = 0.5

		# Defines cells.
		# REF [site] >> https://www.tensorflow.org/tutorials/recurrent
		stacked_cell_fw = tf.contrib.rnn.MultiRNNCell([self._create_cell(num_hidden_units) for _ in range(num_layers)])  # Forward cell.
		stacked_cell_fw = tf.contrib.rnn.DropoutWrapper(stacked_cell_fw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)
		stacked_cell_bw = tf.contrib.rnn.MultiRNNCell([self._create_cell(num_hidden_units) for _ in range(num_layers)])  # Backward cell.
		stacked_cell_bw = tf.contrib.rnn.DropoutWrapper(stacked_cell_bw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Gets cell outputs.
		#cell_outputs, cell_states = tf.nn.bidirectional_dynamic_rnn(stacked_cell_fw, stacked_cell_bw, input_tensor, dtype=tf.float32)
		cell_outputs, _ = tf.nn.bidirectional_dynamic_rnn(stacked_cell_fw, stacked_cell_bw, input_tensor, dtype=tf.float32)
		cell_outputs = tf.concat(cell_outputs, 2)
		#cell_states = tf.concat(cell_states, 2)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_static_rnn(self, input_tensor, is_training_tensor, num_time_steps, num_classes):
		"""
		num_hidden_units = 256
		keep_prob = 1.0
		"""
		num_hidden_units = 512
		keep_prob = 0.5

		# Unstack: a tensor of shape (samples, time-steps, features) -> a list of 'time-steps' tensors of shape (samples, features).
		input_tensor = tf.unstack(input_tensor, num_time_steps, axis=1)

		# Defines a cell.
		cell = self._create_cell(num_hidden_units)
		cell = tf.contrib.rnn.DropoutWrapper(cell, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Gets cell outputs.
		"""
		# REF [site] >> https://www.tensorflow.org/tutorials/recurrent
		#cell_state = cell.zero_state(batch_size, tf.float32)
		cell_state = tf.zeros([batch_size, cell.state_size])
		cell_output_list = []
		probabilities = []
		loss = 0.0
		for inp in input_tensor:
			#cell_output, cell_state = cell(inp, cell_state)
			cell_outputs, _ = cell(inp, cell_state)
			cell_output_list.append(cell_outputs)

			#logits = tf.matmul(cell_output, weights) + biases
			# TODO [check] >>
			logits = tf.layers.dense(cell_output, 1024, activation=tf.nn.softmax, name='fc')
			# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
			logits = tf.layers.dropout(logits, rate=dropout_rate, training=is_training_tensor, name='dropout')

			probabilities.append(tf.nn.softmax(logits))
			loss += loss_function(probabilities, output_tensor[:, i])
		"""
		#cell_outputs, cell_state = tf.nn.static_rnn(cell, input_tensor, dtype=tf.float32)
		cell_outputs, _ = tf.nn.static_rnn(cell, input_tensor, dtype=tf.float32)

		# Stack: a list of 'time-steps' tensors of shape (samples, features) -> a tensor of shape (samples, time-steps, features).
		cell_outputs = tf.stack(cell_outputs, axis=1)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_static_stacked_rnn(self, input_tensor, is_training_tensor, num_time_steps, num_classes):
		num_layers = 2
		"""
		num_hidden_units = 128
		keep_prob = 1.0
		"""
		num_hidden_units = 256
		keep_prob = 0.5

		"""
		# REF [site] >> https://www.tensorflow.org/tutorials/recurrent
		# Defines cells.
		stacked_cells = [self._create_cell(num_hidden_units) for _ in range(num_layers)]

		# Unstack: a tensor of shape (samples, time-steps, features) -> a list of 'time-steps' tensors of shape (samples, features).
		input_tensor = tf.unstack(input_tensor, num_time_steps, axis=1)

		# Gets cell outputs.
		def run_stacked_cells(cells, cell_inputs, cell_state_list):
			cell_outputs = cell_inputs
			new_cell_state_list = []
			for (cell, cell_state) in zip(cells, cell_state_list):
				cell_outputs, cell_state = cell(cell_outputs, cell_state)
				new_cell_state_list.append(cell_state)
			return cell_outputs, new_cell_state_list

		cell_state_list = tf.zeros([[batch_size, cell.state_size] for cell in cells])
		cell_output_list = []
		probabilities = []
		loss = 0.0
		for inp in input_tensor:
			cell_output, cell_state_list = run_stacked_cells(stacked_cells, inp, cell_state_list)
			cell_output_list.append(cell_output)

			#logits = tf.matmul(cell_output, weights) + biases
			# TODO [check] >>
			logits = tf.layers.dense(cell_output, 1024, activation=tf.nn.softmax, name='fc')
			# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
			logits = tf.layers.dropout(logits, rate=dropout_rate, training=is_training_tensor, name='dropout')

			probabilities.append(tf.nn.softmax(logits))
			loss += loss_function(probabilities, output_tensor[:, i])
		"""
		# Defines cells.
		# REF [site] >> https://www.tensorflow.org/tutorials/recurrent
		stacked_cell = tf.contrib.rnn.MultiRNNCell([self._create_cell(num_hidden_units) for _ in range(num_layers)])
		stacked_cell = tf.contrib.rnn.DropoutWrapper(stacked_cell, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Unstack: a tensor of shape (samples, time-steps, features) -> a list of 'time-steps' tensors of shape (samples, features).
		input_tensor = tf.unstack(input_tensor, num_time_steps, axis=1)

		# Gets cell outputs.
		#cell_outputs, cell_state = tf.nn.static_rnn(stacked_cell, input_tensor, dtype=tf.float32)
		cell_outputs, _ = tf.nn.static_rnn(stacked_cell, input_tensor, dtype=tf.float32)

		# Stack: a list of 'time-steps' tensors of shape (samples, features) -> a tensor of shape (samples, time-steps, features).
		cell_outputs = tf.stack(cell_outputs, axis=1)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_static_birnn(self, input_tensor, is_training_tensor, num_time_steps, num_classes):
		"""
		num_hidden_units = 128
		keep_prob = 1.0
		"""
		num_hidden_units = 256
		keep_prob = 0.5

		# Defines cells.
		cell_fw = self._create_cell(num_hidden_units)  # Forward cell.
		cell_fw = tf.contrib.rnn.DropoutWrapper(cell_fw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)
		cell_bw = self._create_cell(num_hidden_units)  # Backward cell.
		cell_bw = tf.contrib.rnn.DropoutWrapper(cell_bw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Unstack: a tensor of shape (samples, time-steps, features) -> a list of 'time-steps' tensors of shape (samples, features).
		input_tensor = tf.unstack(input_tensor, num_time_steps, axis=1)

		# Gets cell outputs.
		#cell_outputs, cell_state_fw, cell_state_bw = tf.nn.static_bidirectional_rnn(cell_fw, cell_bw, input_tensor, dtype=tf.float32)
		#cell_states = tf.concat((cell_state_fw, cell_state_bw), 2)  # ?
		cell_outputs, _, _ = tf.nn.static_bidirectional_rnn(cell_fw, cell_bw, input_tensor, dtype=tf.float32)

		# Stack: a list of 'time-steps' tensors of shape (samples, features) -> a tensor of shape (samples, time-steps, features).
		cell_outputs = tf.stack(cell_outputs, axis=1)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_static_stacked_birnn(self, input_tensor, is_training_tensor, num_time_steps, num_classes):
		num_layers = 2
		"""
		num_hidden_units = 64
		keep_prob = 1.0
		"""
		num_hidden_units = 128
		keep_prob = 0.5

		# Defines cells.
		# REF [site] >> https://www.tensorflow.org/tutorials/recurrent
		stacked_cell_fw = tf.contrib.rnn.MultiRNNCell([self._create_cell(num_hidden_units) for _ in range(num_layers)])  # Forward cell.
		stacked_cell_fw = tf.contrib.rnn.DropoutWrapper(stacked_cell_fw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)
		stacked_cell_bw = tf.contrib.rnn.MultiRNNCell([self._create_cell(num_hidden_units) for _ in range(num_layers)])  # Backward cell.
		stacked_cell_bw = tf.contrib.rnn.DropoutWrapper(stacked_cell_bw, input_keep_prob=keep_prob, output_keep_prob=1.0, state_keep_prob=keep_prob)

		# Unstack: a tensor of shape (samples, time-steps, features) -> a list of 'time-steps' tensors of shape (samples, features).
		input_tensor = tf.unstack(input_tensor, num_time_steps, axis=1)

		# Gets cell outputs.
		#cell_outputs, cell_state_fw, cell_state_bw = tf.nn.static_bidirectional_rnn(stacked_cell_fw, stacked_cell_bw, input_tensor, dtype=tf.float32)
		#cell_states = tf.concat((cell_state_fw, cell_state_bw), 2)  # ?
		cell_outputs, _, _ = tf.nn.static_bidirectional_rnn(stacked_cell_fw, stacked_cell_bw, input_tensor, dtype=tf.float32)

		# Stack: a list of 'time-steps' tensors of shape (samples, features) -> a tensor of shape (samples, time-steps, features).
		cell_outputs = tf.stack(cell_outputs, axis=1)

		#with tf.variable_scope('rnn', reuse=tf.AUTO_REUSE):
		#	dropout_rate = 1 - keep_prob
		#	# NOTE [info] >> If dropout_rate=0.0, dropout layer is not created.
		#	cell_outputs = tf.layers.dropout(cell_outputs, rate=dropout_rate, training=is_training_tensor, name='dropout')

		with tf.variable_scope('fc1', reuse=tf.AUTO_REUSE):
			if 1 == num_classes:
				fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, 1, activation=tf.sigmoid, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			elif num_classes >= 2:
				fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, name='fc')
				#fc1 = tf.layers.dense(cell_outputs, num_classes, activation=tf.nn.softmax, activity_regularizer=tf.contrib.layers.l2_regularizer(0.0001), name='fc')
			else:
				assert num_classes > 0, 'Invalid number of classes.'

			return fc1

	def _create_cell(self, num_units):
		#return tf.contrib.rnn.BasicRNNCell(num_units, forget_bias=1.0)
		return tf.contrib.rnn.BasicLSTMCell(num_units, forget_bias=1.0)
		#return tf.contrib.rnn.RNNCell(num_units, forget_bias=1.0)
		#return tf.contrib.rnn.LSTMCell(num_units, forget_bias=1.0)
		#return tf.contrib.rnn.GRUCell(num_units, forget_bias=1.0)
