#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>


/* Massage */
#define RR_HALL_DBG_ENABLE						0
#define RR_HALL_DBG_FUNC_ENABLE					0
#define RR_HALL_TAG							"[RoadRunner_Hall] "

#define RR_HALL_INFO(...)						\
	do {								\
		printk(KERN_INFO RR_HALL_TAG __VA_ARGS__);		\
	} while (0)

#define RR_HALL_ERR(...)						\
	do {								\
		printk(KERN_ERR RR_HALL_TAG __VA_ARGS__);		\
	} while (0)

#define RR_HALL_DBG(...)						\
	do {								\
		if (RR_HALL_DBG_ENABLE)					\
			printk(KERN_ERR RR_HALL_TAG __VA_ARGS__);	\
	} while (0)

#define RR_HALL_DBG_FUNC						\
	do {								\
		if (RR_HALL_DBG_FUNC_ENABLE) {				\
			printk(KERN_ERR RR_HALL_TAG "%s(): Enter\n",	\
					__func__);			\
		}							\
	} while (0)


/* Driver define */
#define RR_HALL_INT							84
#define RR_HALL_DEBOUNCE						150
#define RR_HALL_INT_NAME						"roadrunner_hall"
#define RR_HALL_DRV_NAME						"roadrunner-hall"


/* Driver data */
enum rr_hall_status {
	RR_HALL_OPENED = 0,
	RR_HALL_CLOSED,
};

struct rr_hall_drvdata {
	struct platform_device *pdev;
	unsigned int irq;
	bool enable;
	struct work_struct work;
	struct mutex lock;
	struct input_dev *input_dev;
	enum rr_hall_status hall_status;
};


/* Driver function */
static int __init rr_hall_init(void);
static void __exit rr_hall_exit(void);
static int rr_hall_driver_probe(struct platform_device *pdev);
static int rr_hall_driver_remove(struct platform_device *pdev);
static void rr_hall_driver_shutdown(struct platform_device *pdev);

static int rr_hall_irq_init(struct rr_hall_drvdata *drvdata);
static int rr_hall_input_dev_init(struct rr_hall_drvdata *drvdata);
static int rr_hall_create_sysfs_file(struct platform_device *pdev);

static irqreturn_t rr_hall_irq_handler(int irq, void *dev_id);
static void rr_hall_task(struct work_struct *w);


/* Platform driver define */
static struct of_device_id rr_hall_of_match[] = {
	{ .compatible = RR_HALL_DRV_NAME, },
};
MODULE_DEVICE_TABLE(of, rr_hall_of_match);

static struct platform_driver rr_hall_driver = {
	.probe = rr_hall_driver_probe,
	.remove = rr_hall_driver_remove,
	.shutdown = rr_hall_driver_shutdown,
	.driver = {
		.name = RR_HALL_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = rr_hall_of_match,
	},
};


/* Driver global variable */
struct rr_hall_drvdata *g_rr_hall_drvdata;


/* Driver implement */
static int __init rr_hall_init(void)
{
	int ret = 0;

	RR_HALL_DBG_FUNC;

	ret = platform_driver_register(&rr_hall_driver);
	if (ret < 0)
		RR_HALL_ERR("Fail to register platform driver\n");

	return ret;
}

static int rr_hall_driver_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct rr_hall_drvdata *drvdata = NULL;

	RR_HALL_DBG_FUNC;

	// Allocate driver data
	drvdata = kzalloc(sizeof(struct rr_hall_drvdata), GFP_KERNEL);
	if (drvdata == NULL) {
		RR_HALL_ERR("Fail to allocate drvdata\n");
		ret = -ENOMEM;
		goto fail_alloc_drvdata;
	}

	// Prepare driver data
	g_rr_hall_drvdata = drvdata;
	dev_set_drvdata(&pdev->dev, drvdata);

	drvdata->pdev = pdev;
	drvdata->enable = true;
	INIT_WORK(&drvdata->work, rr_hall_task);
	mutex_init(&drvdata->lock);
	drvdata->hall_status = RR_HALL_OPENED;

	// Initial irq
	ret = rr_hall_irq_init(drvdata);
	if (ret < 0) {
		RR_HALL_ERR("Fail to init irq\n");
		goto fail_init_irq;
	}

	// Initial input device
	ret = rr_hall_input_dev_init(drvdata);
	if (ret < 0) {
		RR_HALL_ERR("Fail to init input device\n");
		goto fail_init_input_dev;
	}

	// Create sysfs file
	ret = rr_hall_create_sysfs_file(pdev);
	if (ret < 0) {
		RR_HALL_ERR("Fail to create sysfs file\n");
		goto fail_create_sysfs_file;
	}

	return ret;

fail_create_sysfs_file:
fail_init_input_dev:
fail_init_irq:
	kfree(drvdata);
	g_rr_hall_drvdata = drvdata = NULL;

fail_alloc_drvdata:
	return ret;
}

static int rr_hall_irq_init(struct rr_hall_drvdata *drvdata)
{
	int ret = 0;
	unsigned long irq_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	RR_HALL_DBG_FUNC;

	ret = gpio_request(RR_HALL_INT, RR_HALL_INT_NAME);
	if (ret < 0) {
		RR_HALL_ERR("Fail to request irq gpio\n");
		goto fail_request_gpio;
	}

	ret = gpio_direction_input(RR_HALL_INT);
	if (ret < 0) {
		RR_HALL_ERR("Fail to set gpio direction\n");
		goto fail_gpio_direction;
	}

	drvdata->irq = gpio_to_irq(RR_HALL_INT);

	ret = request_threaded_irq(
			drvdata->irq,
			NULL,
			rr_hall_irq_handler,
			irq_flags,
			RR_HALL_INT_NAME,
			drvdata
			);
	if (ret < 0) {
		RR_HALL_ERR("Fail to request irq\n");
		goto fail_request_irq;
	}

	drvdata->hall_status = !gpio_get_value(RR_HALL_INT);

	enable_irq_wake(drvdata->irq);

	return ret;

fail_request_irq:
fail_gpio_direction:
	gpio_free(RR_HALL_INT);

fail_request_gpio:
	return ret;
}

static int rr_hall_input_dev_init(struct rr_hall_drvdata *drvdata)
{
	int ret = 0;

	RR_HALL_DBG_FUNC;

	drvdata->input_dev = input_allocate_device();
	if (! drvdata->input_dev) {
		RR_HALL_ERR("Fail to allocate input device\n");
		ret = -ENOMEM;
		goto fail_alloc_input_dev;
	}

	drvdata->input_dev->name = "RoadRunner Hall";
	drvdata->input_dev->phys = "rr_hall/input0";
	drvdata->input_dev->id.bustype = BUS_HOST;
	drvdata->input_dev->id.vendor = 0x0001;
	drvdata->input_dev->id.product = 0x0001;
	drvdata->input_dev->dev.parent = &drvdata->pdev->dev;

	set_bit(EV_SW, drvdata->input_dev->evbit);
	set_bit(SW_LID, drvdata->input_dev->swbit);

	ret = input_register_device(drvdata->input_dev);
	if (ret < 0) {
		RR_HALL_ERR("Fail to register input device\n");
		goto fail_register_input_dev;
	}

	return ret;

fail_register_input_dev:
	input_free_device(drvdata->input_dev);

fail_alloc_input_dev:
	return ret;
}

static irqreturn_t rr_hall_irq_handler(int irq, void *dev_id)
{
	struct rr_hall_drvdata *drvdata = dev_id;

	RR_HALL_DBG_FUNC;

	// Disable irq
	disable_irq_nosync(drvdata->irq);

	// Schedule the task function
	schedule_work(&drvdata->work);

	return IRQ_HANDLED;
}

static void rr_hall_task(struct work_struct *w)
{
	struct rr_hall_drvdata *drvdata = container_of(w, struct rr_hall_drvdata, work);
	enum rr_hall_status current_hall_status;

	RR_HALL_DBG_FUNC;

	mutex_lock(&drvdata->lock);

	// Debounce
	msleep(RR_HALL_DEBOUNCE);

	// Check hall sensor status
	if (gpio_get_value(RR_HALL_INT))
		current_hall_status = RR_HALL_OPENED;
	else
		current_hall_status = RR_HALL_CLOSED;

	if (current_hall_status == drvdata->hall_status) {
		RR_HALL_DBG("Hall status not changed\n");
		goto finish_do_nothing;
	} else if (! drvdata->enable) {
		drvdata->hall_status = current_hall_status;
		RR_HALL_DBG("Hall sensor disabled\n");
		goto finish_do_nothing;
	}

	// Report hall status
	if (current_hall_status == RR_HALL_OPENED)
		RR_HALL_DBG("Hall status = OPENED\n");
	else
		RR_HALL_DBG("Hall status = CLOSED\n");

	drvdata->hall_status = current_hall_status;

	input_report_switch(drvdata->input_dev, SW_LID, !!drvdata->hall_status);
	input_sync(drvdata->input_dev);

finish_do_nothing:
	enable_irq(drvdata->irq);

	mutex_unlock(&drvdata->lock);
}


/* Driver sysfs */
static ssize_t rr_hall_status_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct rr_hall_drvdata *drvdata = dev_get_drvdata(dev);
	char myBuff[1];

	RR_HALL_DBG_FUNC;

	mutex_lock(&drvdata->lock);

	if (drvdata->hall_status == RR_HALL_OPENED)
		myBuff[0] = '1';
	else
		myBuff[0] = '0';

	memcpy(buf, myBuff, 1);

	mutex_unlock(&drvdata->lock);

	return 1;
}
static DEVICE_ATTR(status, 0444, rr_hall_status_read, NULL);

static ssize_t rr_hall_enable_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct rr_hall_drvdata *drvdata = dev_get_drvdata(dev);

	RR_HALL_DBG_FUNC;

	mutex_lock(&drvdata->lock);

	if (buf[0] == '0') {
		drvdata->enable = false;
		input_report_switch(drvdata->input_dev, SW_LID, 0);
		input_sync(drvdata->input_dev);
	} else
		drvdata->enable = true;

	mutex_unlock(&drvdata->lock);

	return count;
}

static ssize_t rr_hall_enable_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct rr_hall_drvdata *drvdata = dev_get_drvdata(dev);
	char myBuff[1];

	RR_HALL_DBG_FUNC;

	mutex_lock(&drvdata->lock);

	if (drvdata->enable)
		myBuff[0] = '1';
	else
		myBuff[0] = '0';

	memcpy(buf, myBuff, 1);

	mutex_unlock(&drvdata->lock);

	return 1;
}
static DEVICE_ATTR(enable, 0660, rr_hall_enable_read, rr_hall_enable_write);

static int rr_hall_create_sysfs_file(struct platform_device *pdev)
{
	int ret = 0;

	RR_HALL_DBG_FUNC;

	ret = device_create_file(&pdev->dev, &dev_attr_status);
	if (ret < 0) {
		RR_HALL_ERR("Fail to create sysfs file\n");
		goto fail_dev_create_file;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_enable);
	if (ret < 0) {
		RR_HALL_ERR("Fail to create sysfs file\n");
		goto fail_dev_create_file;
	}

	return ret;

fail_dev_create_file:
	return -1;
}

static int rr_hall_driver_remove(struct platform_device *pdev)
{
	struct rr_hall_drvdata *drvdata = dev_get_drvdata(&pdev->dev);

	RR_HALL_DBG_FUNC;

	if (g_rr_hall_drvdata) {
		input_unregister_device(drvdata->input_dev);
		free_irq(drvdata->irq, drvdata);
		gpio_free(RR_HALL_INT);
		g_rr_hall_drvdata = drvdata = NULL;
	}

	return 0;
}

static void rr_hall_driver_shutdown(struct platform_device *pdev)
{
	struct rr_hall_drvdata *drvdata = dev_get_drvdata(&pdev->dev);

	RR_HALL_DBG_FUNC;

	if (g_rr_hall_drvdata) {
		input_unregister_device(drvdata->input_dev);
		free_irq(drvdata->irq, drvdata);
		gpio_free(RR_HALL_INT);
		g_rr_hall_drvdata = drvdata = NULL;
	}
}

static void __exit rr_hall_exit(void)
{
	RR_HALL_DBG_FUNC;

	platform_driver_unregister(&rr_hall_driver);
}


module_init(rr_hall_init);
module_exit(rr_hall_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Cheng <chris.cheng@quantatw.com>");
MODULE_DESCRIPTION("RoadRunner hall sensor driver");

