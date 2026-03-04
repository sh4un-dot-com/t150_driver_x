static inline int t150_init_attributes(struct t150 *t150)
{
	int errno;

	/* before exposing sysfs we try to fetch the current configuration from the
	 * wheel: this keeps the cache in sync so that the show methods can simply
	 * read the cached values.  the call is cheap enough that doing it in probe
	 * is acceptable. */
	t150_setup_task(t150);

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_autocenter);
	if(errno)
		return errno;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_enable_autocenter);
	if(errno)
		goto err1;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_range);
	if(errno)
		goto err2;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_gain);
	if(errno)
		goto err3;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_firmware_version);
	if(errno)
		goto err4;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_max_range);
	if (errno)
		goto err5;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_settings_cache_age_ms);
	if (errno)
		goto err6;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_supported_ff_effects);
	if (errno)
		goto err7;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_reload_settings);
	if (errno)
		goto err8;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_firmware_upgrade);
	if (errno)
		goto err9;

	return 0;

err9: device_remove_file(&t150->hid_device->dev, &dev_attr_reload_settings);
err8: device_remove_file(&t150->hid_device->dev, &dev_attr_supported_ff_effects);
err7: device_remove_file(&t150->hid_device->dev, &dev_attr_settings_cache_age_ms);
err6: device_remove_file(&t150->hid_device->dev, &dev_attr_max_range);
err5: device_remove_file(&t150->hid_device->dev, &dev_attr_firmware_version);
err4: device_remove_file(&t150->hid_device->dev, &dev_attr_gain);
err3: device_remove_file(&t150->hid_device->dev, &dev_attr_range);
err2: device_remove_file(&t150->hid_device->dev, &dev_attr_enable_autocenter);
err1: device_remove_file(&t150->hid_device->dev, &dev_attr_autocenter);
	return errno;
}

static inline void t150_free_attributes(struct t150 *t150)
{
	device_remove_file(&t150->hid_device->dev, &dev_attr_autocenter);
	device_remove_file(&t150->hid_device->dev, &dev_attr_enable_autocenter);
	device_remove_file(&t150->hid_device->dev, &dev_attr_range);
	device_remove_file(&t150->hid_device->dev, &dev_attr_gain);
	device_remove_file(&t150->hid_device->dev, &dev_attr_firmware_version);
	device_remove_file(&t150->hid_device->dev, &dev_attr_max_range);
	device_remove_file(&t150->hid_device->dev, &dev_attr_settings_cache_age_ms);
	device_remove_file(&t150->hid_device->dev, &dev_attr_supported_ff_effects);
	device_remove_file(&t150->hid_device->dev, &dev_attr_reload_settings);
	device_remove_file(&t150->hid_device->dev, &dev_attr_firmware_upgrade);
}

/**/
static ssize_t t150_store_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t nforce;
	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &nforce))
		return -EINVAL;

	t150_set_autocenter(t150, nforce);

	return count;
}

static ssize_t t150_show_return_force(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);

	/* refresh cache before returning so userspace sees the value stored in the
	 * wheel even if it was modified by hardware buttons. */
	if (t150_refresh_settings_cached(t150, false) < 0)
		return -EIO;

	/* use getter helper so the cached-access code is centralised */
	{
		uint16_t val = 0;
		t150_get_autocenter(t150, &val);
		len = sprintf(buf, "%d\n", val);
	}

	return len;
}

static ssize_t t150_store_simulate_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	bool use;
	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(!kstrtobool(buf, &use))
		t150_set_enable_autocenter(t150, use);	
	else
		return -EINVAL;

	return count;
}

static ssize_t t150_show_simulate_return_force(struct device *dev, struct device_attribute *attr,char * buf)
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);
    
	if (t150_refresh_settings_cached(t150, false) < 0)
		return -EIO;
	{
		bool en = false;
		t150_get_enable_autocenter(t150, &en);
		len = sprintf(buf, "%c\n", en ? 'y' : 'n');
	}

	return len;
}

static ssize_t t150_store_range(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t range;
	uint16_t dev_max_range = 0;

	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &range))
		return -EINVAL;

	if(t150->hid_device->product == USB_T150_PRODUCT_ID)
		dev_max_range = 1080;
	else if (t150->hid_device->product == USB_TMX_PRODUCT_ID)
		dev_max_range = 900;
	else
		return -EINVAL;

	if(range < 270)
		range = 270;
	else if (range > dev_max_range)
		range = dev_max_range;

	range = DIV_ROUND_CLOSEST((range * 0xffff), dev_max_range);

	t150_set_range(t150, range);

	return count;
}

static ssize_t t150_show_range(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);

	uint16_t dev_max_range = 0;

	/* make sure cached range is up to date */
	if (t150_refresh_settings_cached(t150, false) < 0)
		return -EIO;

	if (t150_get_max_range(t150, &dev_max_range) < 0)
		return -EINVAL;

	{
		uint16_t r = 0;
		t150_get_range(t150, &r);
		len = sprintf(buf, "%d\n", DIV_ROUND_CLOSEST(r * dev_max_range, 0xffff));
	}

	return len;
}

static ssize_t t150_store_ffb_intensity(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t nforce;
	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &nforce))
		return -EINVAL;

	t150_set_gain(t150, nforce);
	return count;
}

static ssize_t t150_show_ffb_intensity(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);

	/* refresh in case an FFB-capable userland tool changed the gain
	 * directly */
	if (t150_refresh_settings_cached(t150, false) < 0)
		return -EIO;

	{
		uint16_t g = 0;
		t150_get_gain(t150, &g);
		len = sprintf(buf, "%d\n", g);
	}

	return len;
}

ssize_t t150_show_fw_version(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);

	/* firmware version is also supplied by t150_read_settings */
	if (t150_refresh_settings_cached(t150, false) < 0)
		return -EIO;

	{
		uint16_t v = 0;
		/* firmware id is a byte but stored in the cached struct */
		unsigned long flags;
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		v = t150->settings.firmware_version;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
		len = sprintf(buf, "%d\n", v);
	}

	return len;
}

/* write-only attribute that causes the driver to re-poll the hardware
 * and update the cached settings.  writing any value (even the empty
 * string) triggers the refresh. */
static ssize_t t150_store_reload_settings(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct t150 *t150 = dev_get_drvdata(dev);

	/* ignore content, just update the cached copy */
	if (t150_refresh_settings_cached(t150, true) < 0)
		return -EIO;
	return count;
}

static ssize_t t150_show_max_range(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t max_range;
	struct t150 *t150 = dev_get_drvdata(dev);

	if (t150_get_max_range(t150, &max_range) < 0)
		return -EINVAL;

	return sprintf(buf, "%u\n", max_range);
}

static ssize_t t150_show_settings_cache_age_ms(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct t150 *t150 = dev_get_drvdata(dev);
	unsigned long flags;
	unsigned long age_ms;
	bool cache_valid;
	unsigned long cache_updated_jiffies;

	t150_refresh_settings_cached(t150, false);

	spin_lock_irqsave(&t150->settings.access_lock, flags);
	cache_valid = t150->settings.cache_valid;
	cache_updated_jiffies = t150->settings.cache_updated_jiffies;
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);

	if (!cache_valid)
		return sprintf(buf, "-1\n");

	age_ms = jiffies_to_msecs(jiffies - cache_updated_jiffies);
	return sprintf(buf, "%lu\n", age_ms);
}

static ssize_t t150_show_supported_ff_effects(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,
		"FF_GAIN,FF_PERIODIC,FF_SINE,FF_SAW_UP,FF_SAW_DOWN,FF_CONSTANT,FF_SPRING,FF_DAMPER\n");
}

/* placeholder for firmware upload; the protocol is proprietary and the
 * implementation depends on having a firmware image unpacked, so for now
 * the code just prints a notice.  real upgrade code would split the
 * buffer into packets and send them with a vendor control URB. */
static ssize_t t150_store_firmware_upgrade(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct t150 *t150 = dev_get_drvdata(dev);

	hid_warn(t150->hid_device,
		"firmware upgrade invoked (%zu bytes) - not implemented\n", count);
	/* drop data; in a real driver you would parse the firmware image and
	 * transmit it to the wheel, possibly taking it out of normal operating
	 * mode first. */
	return count;
}
