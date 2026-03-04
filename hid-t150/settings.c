/**
 * @param t150 ptr to t150
 * @param gain a value between 0x00 and 0xffff where 0xffff is 100% gain
 * @return 0 on success @see usb_interrupt_msg for return codes
 */
static int t150_set_gain(struct t150 *t150, uint16_t gain)
{
	int boh, errno;
	struct ff_change_gain buffer;
	unsigned long flags;

	buffer.f0 = 0x43;
	buffer.gain = cpu_to_le16(gain);

	mutex_lock(&t150->lock);

	// Send to the wheel desidered return force
	errno = usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&buffer,
		sizeof(buffer), &boh,
		SETTINGS_TIMEOUT
	);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.gain = gain;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	} else {
		hid_err(t150->hid_device, "Operation set gain failed with code %d", errno);
	}

	mutex_unlock(&t150->lock);

	return errno;
}

/**
 * @param autocenter_force a value between 0 and 65535, is the strength of the autocenter effect
 */
static __always_inline int t150_set_autocenter(struct t150 *t150, uint16_t autocenter_force)
{
	struct operation40 buffer;
	int errno;
	unsigned long flags;

	mutex_lock(&t150->lock);

	errno = t150_settings_set40(t150, SET40_RETURN_FORCE, DIV_ROUND_CLOSEST((autocenter_force * 100), 0xffff), &buffer);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.autocenter_force = autocenter_force;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	}

	mutex_unlock(&t150->lock);
	return errno;
}

/**
 * @param enable true if the autocenter effect is to be kept enabled when the input
 * 	is opened. The autocentering effect is always active while no input are open
 */
static __always_inline int t150_set_enable_autocenter(struct t150 *t150, bool enable)
{
	struct operation40 buffer;
	int errno;
	unsigned long flags;

	mutex_lock(&t150->lock);

	errno = t150_settings_set40(t150, SET40_USE_RETURN_FORCE, enable, &buffer);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.autocenter_enabled = enable;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	}

	mutex_unlock(&t150->lock);
	return errno;
}

/**
 * @param range a value between 0x0000 and 0xffff where 0xffff is 1080°
 * 	wheel range
 */
static __always_inline int t150_set_range(struct t150 *t150, uint16_t range)
{
	struct operation40 buffer;
	int errno;
	unsigned long flags;

	mutex_lock(&t150->lock);

	errno = t150_settings_set40(t150, SET40_RANGE, range, &buffer);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.range = range;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	}

	mutex_unlock(&t150->lock);
	return errno;
}


/**
 * @t150 pointer to t150
 * @operation number of operation
 * @argument the argument to pass with the request
 * @buffer a buffer of at least 4 BYTES!
 * @return 0 on success @see usb_interrupt_msg for return codes
 */
static int t150_settings_set40(
	struct t150 *t150, operation_t operation, uint16_t argument, void *buffer_
)
{
	int boh, errno;
	struct operation40 *buffer = buffer_;
	buffer->code = 0x40;
	buffer->operation = operation;
	buffer->argument = cpu_to_le16(argument);

	// Send to the wheel desidered return force
	errno = usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		buffer,
		sizeof(struct operation40), &boh,
		SETTINGS_TIMEOUT
	);

	if (errno)
		hid_err(t150->hid_device,
				"errno %d during operation 0x40 0x%02hhX with argument (big endian) %04hX",
				errno, operation, argument);

	return errno;
}


/* pull the wheel's current configuration block and cache the values */
static int t150_read_settings(struct t150 *t150)
{
	int ret;
	uint8_t buf[8];

	mutex_lock(&t150->lock);
	ret = usb_control_msg(
		t150->usb_device,
		usb_rcvctrlpipe(t150->usb_device, 0),
		86,          /* request used by setup task */
		0xc1,        /* vendor | device‑to‑host */
		0, 0,
		buf, sizeof(buf),
		SETTINGS_TIMEOUT
	);
	mutex_unlock(&t150->lock);

	if (ret < 0)
		return ret;
	if (ret != sizeof(buf))
		return -EIO;

	spin_lock_irq(&t150->settings.access_lock);
	t150->settings.firmware_version = buf[1];
	/* layout guessed from captures; the remaining words seem to
	 * correspond to gain, autocenter and range in little endian */
	t150->settings.gain = make_word(buf[2], buf[3]);
	t150->settings.autocenter_force = make_word(buf[4], buf[5]);
	t150->settings.range = make_word(buf[6], buf[7]);
	spin_unlock_irq(&t150->settings.access_lock);

	return 0;
}

/* convenience helpers that read the cached copy */
static int t150_get_gain(struct t150 *t150, uint16_t *gain)
{
	unsigned long flags;
	spin_lock_irqsave(&t150->settings.access_lock, flags);
	*gain = t150->settings.gain;
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	return 0;
}

static int t150_get_autocenter(struct t150 *t150, uint16_t *autocenter_force)
{
	unsigned long flags;
	spin_lock_irqsave(&t150->settings.access_lock, flags);
	*autocenter_force = t150->settings.autocenter_force;
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	return 0;
}

static int t150_get_enable_autocenter(struct t150 *t150, bool *enable)
{
	unsigned long flags;
	spin_lock_irqsave(&t150->settings.access_lock, flags);
	*enable = t150->settings.autocenter_enabled;
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	return 0;
}

static int t150_get_range(struct t150 *t150, uint16_t *range)
{
	unsigned long flags;
	spin_lock_irqsave(&t150->settings.access_lock, flags);
	*range = t150->settings.range;
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	return 0;
}

static int t150_setup_task(struct t150 *t150)
{
	int errno = 0;

	/* try reading whatever is already stored in the wheel; if the
	 * transaction fails we fall back to hard‑coded defaults */
	errno = t150_read_settings(t150);
	if (errno < 0) {
		hid_warn(t150->hid_device,
			"unable to query wheel settings (%d), using defaults\n", errno);
	} else {
		hid_info(t150->hid_device,
			"settings read from wheel: gain=%u autocenter=%u range=%u fw=%u\n",
			t150->settings.gain,
			t150->settings.autocenter_force,
			t150->settings.range,
			t150->settings.firmware_version);
		/* we already have the values, don't override */
		return 0;
	}

	/* original defaulting logic */
	errno = t150_set_gain(t150, 0xbffe); // ~75%
	if(errno)
		hid_err(t150->hid_device,
			"Error %d while setting the t150 default gain\n", errno);

	errno = t150_set_enable_autocenter(t150, false);
	if(errno)
		hid_err(t150->hid_device,
			"Error %d while setting the t150 default enable_autocenter\n", errno);

	errno = t150_set_autocenter(t150, 0x7fff);
	if(errno)
		hid_err(t150->hid_device,
			"Error %d while setting the t150 default autocenter\n", errno);

	errno = t150_set_range(t150, 0xffff);
	if(errno)
		hid_err(t150->hid_device,
			"Error %d while setting the t150 default range\n", errno);

	hid_info(t150->hid_device,
		"Setup completed! Firmware version is %d\n",
		t150->settings.firmware_version);

	return errno;
}
