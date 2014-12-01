
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>

extern unsigned char HWID;

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>

#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define MXT_SUSPEND_LEVEL 1
#endif

struct isl98607_platform_data {
	int lcd_pwr_en;
	int lcd_pump_enp;
	int lcd_pump_enn;
};

static int isl98607_parse_dt(struct device *dev, struct isl98607_platform_data *pdata)
{
	int rc;
	struct device_node  *np = dev->of_node;

	pr_debug("isl98607_parse_dt + \n");
	pdata->lcd_pwr_en   = of_get_named_gpio(np, "isl,lcd_pwr_en",   0);
	if (!gpio_is_valid(pdata->lcd_pwr_en)) {
		pr_debug("%s:%d, lcd_pwr_en gpio not specified\n", __func__, __LINE__);
	}
	else {
		rc = gpio_request(pdata->lcd_pwr_en, "lcd_pwr_en");
		if (rc) {
			pr_debug("request lcd_pwr_en gpio failed, rc=%d\n", rc);
			gpio_free(pdata->lcd_pwr_en);
			if (gpio_is_valid(pdata->lcd_pwr_en))
				gpio_free(pdata->lcd_pwr_en);
			return -ENODEV;
		}
	}

	pdata->lcd_pump_enp = of_get_named_gpio(np, "isl,lcd_pump_enp", 0);
	if (!gpio_is_valid(pdata->lcd_pump_enp)) {
		pr_debug("%s:%d, lcd_pump_enp gpio not specified\n", __func__, __LINE__);
	}
	else {
		rc = gpio_request(pdata->lcd_pump_enp, "lcd_pump_enp");
		if (rc) {
			pr_debug("request lcd_pump_enp gpio failed, rc=%d\n", rc);
			gpio_free(pdata->lcd_pump_enp);
			if (gpio_is_valid(pdata->lcd_pump_enp))
				gpio_free(pdata->lcd_pump_enp);
			return -ENODEV;
		}
	}

	pdata->lcd_pump_enn = of_get_named_gpio(np, "isl,lcd_pump_enn", 0);
	if (!gpio_is_valid(pdata->lcd_pump_enn)) {
		pr_debug("%s:%d, lcd_pump_enn gpio not specified\n", __func__, __LINE__);
	}
	else {
		rc = gpio_request(pdata->lcd_pump_enn, "lcd_pump_enn");
		if (rc) {
			pr_debug("request lcd_pump_enn gpio failed, rc=%d\n", rc);
			gpio_free(pdata->lcd_pump_enn);
			if (gpio_is_valid(pdata->lcd_pump_enn))
				gpio_free(pdata->lcd_pump_enn);
			return -ENODEV;
		}
	}
	pr_debug("isl98607_parse_dt - \n");
	return 0;
}

static int isl98607_config_gpio(struct device *dev, struct isl98607_platform_data *pdata)
{
	pr_debug("isl98607_config_gpio + \n");

	pr_debug("isl98607_config_gpio, lcd_pwr_en =%d\n", pdata->lcd_pwr_en);
	if (gpio_is_valid(pdata->lcd_pwr_en)) {
		pr_debug("isl98607_config_gpio lcd_pwr_en, set 1\n");
		gpio_set_value(pdata->lcd_pwr_en, 1);
	}
	msleep(1);
	pr_debug("isl98607_config_gpio, lcd_pump_enp =%d\n", pdata->lcd_pump_enp);
	if (gpio_is_valid(pdata->lcd_pump_enp))
	{
		pr_debug("isl98607_config_gpio lcd_pump_enp, set 1\n");
		gpio_set_value(pdata->lcd_pump_enp, 1);
	}
	msleep(1);
	pr_debug("isl98607_config_gpio, lcd_pump_enn =%d\n", pdata->lcd_pump_enn);
	if (gpio_is_valid(pdata->lcd_pump_enn))
	{
		pr_debug("isl98607_config_gpio lcd_pump_enn, set 1\n");
		gpio_set_value(pdata->lcd_pump_enn, 1);
	}

	if(HWID == 0 || HWID == 1){
		printk("Enable GPS external LNA EN pin\n");
		gpio_request(63, "gps_lna_en");
		gpio_set_value(63, 1);
	}

	pr_debug("isl98607_config_gpio - \n");
	return 0;
}

#if 0
#ifdef CONFIG_PM
static int isl98607_suspend(struct device *dev)
{
 pr_debug("isl98607_suspend\n");
	return 0;
};
static int isl98607_resume(struct device *dev)
{
 pr_debug("isl98607_resume\n");
	return 0;
};

static const struct dev_pm_ops isl98607_pm_ops = {
#if (!defined(CONFIG_FB) && !defined(CONFIG_HAS_EARLYSUSPEND))
	.suspend	= isl98607_suspend,
	.resume		= isl98607_resume,
#endif
};
#else
static int isl98607_suspend(struct device *dev)
{
	return 0;
};
static int isl98607_resume(struct device *dev)
{
	return 0;
};
#endif
#endif

static int __devinit isl98607_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct isl98607_platform_data *pdata;
	int error;
	pr_debug("isl98607_probe\n");
 
	if (client->dev.of_node) {
	pdata = devm_kzalloc(&client->dev,
                       sizeof(struct isl98607_platform_data), GFP_KERNEL);
	if (!pdata) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	error = isl98607_parse_dt(&client->dev, pdata);
	if (error)
		return error;
	}
	else
	  pdata = client->dev.platform_data;

	if (!pdata)
		return -EINVAL;

	error = isl98607_config_gpio(&client->dev, pdata);

	return 0;
}

static int __devexit isl98607_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isl_id[] = {
	{ "isl98607", 0 },
};
MODULE_DEVICE_TABLE(i2c, isl_id);

#ifdef CONFIG_OF
static struct of_device_id isl98607_match_table[] = {
	{ .compatible = "isl,isl98607",},
	{ },
};
#else
#define isl98607_match_table NULL
#endif


static struct i2c_driver isl98607_driver = {
	.driver = {
		.name	= "isl_lcd_pmu",
		.owner	= THIS_MODULE,
		.of_match_table = isl98607_match_table,
#ifdef CONFIG_PM
//		.pm	= &isl98607_pm_ops,
#endif
	},
	.probe		= isl98607_probe,
	.remove		= __devexit_p(isl98607_remove),
	.id_table	= isl_id,
};



module_i2c_driver(isl98607_driver);

/* Module information */
MODULE_AUTHOR("Raby <raby.lin@quantatw.com>");
MODULE_DESCRIPTION("ISL98607 lcd pmu driver");
MODULE_LICENSE("GPL");

