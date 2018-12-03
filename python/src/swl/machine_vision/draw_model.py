import tensorflow as tf
from swl.machine_learning.tensorflow.tf_neural_net import TensorFlowNeuralNet, TensorFlowBasicSeq2SeqNeuralNet, TensorFlowSeq2SeqNeuralNet

#%%------------------------------------------------------------------

class DRAW(object):
	def __init__(self, image_height, image_width, num_time_steps, use_read_attention, use_write_attention, eps=1e-8):
		super().__init__()

		self._image_height, self._image_width = image_height, image_width
		self._num_time_steps = num_time_steps
		#self._reuse = None
		self._reuse = tf.AUTO_REUSE

		self._img_size = self._image_height * self._image_width  # The canvas size.
		self._eps = eps  # epsilon for numerical stability.

		self._read_n = 5  # Read glimpse grid width/height.
		self._write_n = 5  # Write glimpse grid width/height.
		#self._read_size = 2 * self._read_n * self._read_n if use_read_attention else 2 * self._img_size
		self._write_size = self._write_n * self._write_n if use_write_attention else self._img_size

		self._read_op = self._read_with_attention if use_read_attention else self._read_without_attention
		self._write_op = self._write_with_attention if use_write_attention else self._write_without_attention

		#--------------------
		# FIXME [recover] >>
		#self._input_shape = (batch_size, self._img_size)
		self._input_shape = (100, self._img_size)
		self._input_tensor_ph = tf.placeholder(tf.float32, shape=self._input_shape, name='input_tensor_ph')  # (batch_size * image_size).
		# TODO [implement] >>
		#self._batch_size_ph = tf.placeholder(tf.int32, [1], name='batch_size_ph')
		#self._is_training_tensor_ph = tf.placeholder(tf.bool, name='is_training_tensor_ph')

		# model_output is used in training, evaluation, and inference steps.
		self._model_output = None

		# Loss and accuracy are used in training and evaluation steps.
		self._loss = None
		self._accuracy = None

	@property
	def model_output(self):
		if self._model_output is None:
			raise TypeError
		return self._model_output

	@property
	def loss(self):
		if self._loss is None:
			raise TypeError
		return self._loss

	@property
	def accuracy(self):
		return self._accuracy

	def get_feed_dict(self, data, **kwargs):
		feed_dict = {self._input_tensor_ph: data}
		return feed_dict

	def create_training_model(self):
		batch_size = self._input_shape[0]
		self._model_output, mus, logsigmas, sigmas = self._create_single_model(self._input_tensor_ph, batch_size, self._num_time_steps, True)

		self._loss = self._get_loss(self._model_output, self._input_tensor_ph, mus, logsigmas, sigmas)
		self._accuracy = None

	def create_evaluation_model(self):
		raise NotImplementedError

	def create_inference_model(self):
		batch_size = self._input_shape[0]
		self._model_output, _, _, _ = self._create_single_model(self._input_tensor_ph, batch_size, self._num_time_steps, False)

		self._loss = None
		self._accuracy = None

	def _create_single_model(self, x, batch_size, num_time_steps, is_training):
		with tf.variable_scope('draw_model', reuse=tf.AUTO_REUSE):
			#self._reuse = None  # Workaround for variable_scope(reuse=True).

			# Number of hidden units / output size in LSTM.
			enc_size, dec_size = 256, 256
			z_size = 10  # Q-sampler output size.

			e = tf.random_normal((batch_size, z_size), mean=0, stddev=1)  # Q-sampler noise.
			lstm_enc = tf.contrib.rnn.LSTMCell(enc_size, state_is_tuple=True)  # Encoder op.
			lstm_dec = tf.contrib.rnn.LSTMCell(dec_size, state_is_tuple=True)  # Decoder op.

			canvases = [0] * num_time_steps  # Sequence of canvases.
			mus, logsigmas, sigmas = [0] * num_time_steps, [0] * num_time_steps, [0] * num_time_steps  # Gaussian params generated by SampleQ. We will need these for computing loss.

			# Initial states.
			h_dec_prev = tf.zeros((batch_size, dec_size))
			enc_state = lstm_enc.zero_state(batch_size, tf.float32)
			dec_state = lstm_dec.zero_state(batch_size, tf.float32)

			# DRAW model.
			# Construct the unrolled computational graph.
			for t in range(num_time_steps):
				c_prev = tf.zeros((batch_size, self._img_size)) if 0 == t else canvases[t-1]
				x_hat = x - tf.sigmoid(c_prev)  # Error image.
				r = self._read_op(x, x_hat, h_dec_prev)
				h_enc, enc_state = self._encode(lstm_enc, enc_state, tf.concat([r, h_dec_prev], 1))
				z, mus[t], logsigmas[t], sigmas[t] = self._sampleQ(h_enc, z_size, e)
				h_dec, dec_state = self._decode(lstm_dec, dec_state, z)
				canvases[t] = c_prev + self._write_op(h_dec)  # Store results.
				h_dec_prev = h_dec

				#self._reuse = True  # From now on, share variables.

		return canvases, mus, logsigmas, sigmas

	def _get_loss(self, canvases, x, mus, logsigmas, sigmas):
		with tf.variable_scope('loss', reuse=tf.AUTO_REUSE):
			# Reconstruction loss.
			# Reconstruction term appears to have been collapsed down to a single scalar value (rather than one per item in minibatch).
			x_recons = tf.nn.sigmoid(canvases[-1])

			# After computing binary cross entropy, sum across features then take the mean of those sums across minibatches.
			Lx = tf.reduce_sum(DRAW._binary_crossentropy(x, x_recons, self._eps), 1)  # Reconstruction term.
			Lx = tf.reduce_mean(Lx)

			# Latent loss.
			kl_terms = [0] * self._num_time_steps
			for t in range(self._num_time_steps):
				mu2 = tf.square(mus[t])
				sigma2 = tf.square(sigmas[t])
				logsigma = logsigmas[t]
				kl_terms[t] = 0.5 * tf.reduce_sum(mu2 + sigma2 - 2 * logsigma, 1) - 0.5  # Each kl term is (1 x minibatch).
			KL = tf.add_n(kl_terms)  # This is 1 x minibatch, corresponding to summing kl_terms from 1:num_time_steps.
			Lz = tf.reduce_mean(KL)  # Average over minibatches.

			loss = Lx + Lz

			tf.summary.scalar('loss', loss)
			return loss

	@classmethod
	def _binary_crossentropy(cls, t, o, eps):
		return -(t * tf.log(o + eps) + (1.0 - t) * tf.log(1.0 - o + eps))

	@classmethod
	def _linear_transform(cls, x, output_dim):
		"""
		Affine transformation W * x + b.
		Assumes x.shape = (batch_size, num_features).
		"""
		W = tf.get_variable('W', [x.get_shape()[1], output_dim]) 
		b = tf.get_variable('b', [output_dim], initializer=tf.constant_initializer(0.0))
		return tf.matmul(x, W) + b

	def _filterbank(self, gx, gy, sigma2, delta, N):
		grid_i = tf.reshape(tf.cast(tf.range(N), tf.float32), [1, -1])
		mu_x = gx + (grid_i - N / 2 - 0.5) * delta  # Eqn 19.
		mu_y = gy + (grid_i - N / 2 - 0.5) * delta  # Eqn 20.
		a = tf.reshape(tf.cast(tf.range(self._image_width), tf.float32), [1, 1, -1])
		b = tf.reshape(tf.cast(tf.range(self._image_height), tf.float32), [1, 1, -1])
		mu_x = tf.reshape(mu_x, [-1, N, 1])
		mu_y = tf.reshape(mu_y, [-1, N, 1])
		sigma2 = tf.reshape(sigma2, [-1, 1, 1])
		Fx = tf.exp(-tf.square(a - mu_x) / (2 * sigma2))
		Fy = tf.exp(-tf.square(b - mu_y) / (2 * sigma2))  # batch_size x N x image_height.
		# Normalize, sum over image_width and image_height dims.
		Fx = Fx / tf.maximum(tf.reduce_sum(Fx, 2, keep_dims=True), self._eps)
		Fy = Fy / tf.maximum(tf.reduce_sum(Fy, 2, keep_dims=True), self._eps)
		return Fx, Fy

	def _attention_window(self, scope, h_dec, N):
		with tf.variable_scope(scope, reuse=self._reuse):
			params = DRAW._linear_transform(h_dec, 5)  # 5 parameters.
		#gx_tilde, gy_tilde, log_sigma2, log_delta, log_gamma = tf.split(1, 5, params)
		gx_tilde, gy_tilde, log_sigma2, log_delta, log_gamma = tf.split(params, 5, 1)
		gx = (self._image_width + 1) / 2 * (gx_tilde + 1)
		gy = (self._image_height + 1) / 2 * (gy_tilde + 1)
		sigma2 = tf.exp(log_sigma2)
		delta = (max(self._image_width, self._image_height) - 1) / (N - 1) * tf.exp(log_delta)  # batch_size x N.
		return self._filterbank(gx, gy, sigma2, delta, N) + (tf.exp(log_gamma),)

	# Reader.
	def _read_without_attention(self, x, x_hat, h_dec_prev):
		return tf.concat([x, x_hat], 1)

	def _filter_image(self, img, Fx, Fy, gamma, N):
		Fxt = tf.transpose(Fx, perm=[0, 2, 1])
		img = tf.reshape(img, [-1, self._image_height, self._image_width])
		glimpse = tf.matmul(Fy, tf.matmul(img, Fxt))
		glimpse = tf.reshape(glimpse, [-1, N * N])
		return glimpse * tf.reshape(gamma, [-1, 1])

	def _read_with_attention(self, x, x_hat, h_dec_prev):
		Fx, Fy, gamma = self._attention_window('read_attention', h_dec_prev, self._read_n)
		x = self._filter_image(x, Fx, Fy, gamma, self._read_n)  # batch_size x (read_n * read_n).
		x_hat = self._filter_image(x_hat, Fx, Fy, gamma, self._read_n)
		return tf.concat([x, x_hat], 1)  # Concat along feature axis.

	# Encoder. 
	def _encode(self, lstm_enc, state, input):
		"""
		Run LSTM.
		state = previous encoder state
		input = cat(read, h_dec_prev)
		returns: (output, new_state)
		"""
		with tf.variable_scope('encoder', reuse=self._reuse):
			return lstm_enc(input, state)

	# Q-sampler (variational autoencoder).
	def _sampleQ(self, h_enc, z_size, e):
		"""
		Samples Zt ~ normrnd(mu, sigma) via reparameterization trick for normal dist.
		mu is (batch_size, z_size).
		"""
		with tf.variable_scope('mu', reuse=self._reuse):
			mu = DRAW._linear_transform(h_enc, z_size)
		with tf.variable_scope('sigma', reuse=self._reuse):
			logsigma = DRAW._linear_transform(h_enc, z_size)
			sigma = tf.exp(logsigma)
		return (mu + sigma * e, mu, logsigma, sigma)

	# Decoder.
	def _decode(self, lstm_dec, state, input):
		with tf.variable_scope('decoder', reuse=self._reuse):
			return lstm_dec(input, state)

	# Writer.
	def _write_with_attention(self, h_dec):
		with tf.variable_scope('write_without_attention', reuse=self._reuse):
			return DRAW._linear_transform(h_dec, self._img_size)

	def _write_with_attention(self, h_dec):
		with tf.variable_scope('write_with_attention', reuse=self._reuse):
			w = DRAW._linear_transform(h_dec, self._write_size)  # batch_size x (write_n * write_n).
		batch_size = self._input_shape[0]
		w = tf.reshape(w, [batch_size, self._write_n, self._write_n])
		Fx, Fy, gamma = self._attention_window('write_attention', h_dec, self._write_n)
		Fyt = tf.transpose(Fy, perm=[0, 2, 1])
		wr = tf.matmul(Fyt, tf.matmul(w, Fx))
		wr = tf.reshape(wr, [batch_size, self._image_height * self._image_width])
		#gamma = tf.tile(gamma, [1, self._image_height * self._image_width])
		return wr * tf.reshape(1.0 / gamma, [-1, 1])
