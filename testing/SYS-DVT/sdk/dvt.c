/*
 * dvt.c - TIP GPIO access module
 *
 *  Copyright (C) 2015 Nuranwireless Inc.
 *  <support@nuranwireless.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 #include <linux/interrupt.h>
 #include <linux/err.h>
 #include <linux/module.h>
 #include <linux/types.h>
 #include <linux/platform_device.h>
 #include <linux/io.h>
 #include <linux/of_gpio.h>
 #include <linux/of.h>
 #include <linux/sysfs.h>
 
struct dvt_priv
{
	struct device *dev;

    int bb_current_temp_sensor_alert_gpio;
	char bb_current_temp_sensor_alert_szGpio[32];
	u8 bb_current_temp_sensor_alert_val;
	u32 bb_current_temp_sensor_alert_default;

    unsigned int bb_curr_temp_irq_line;
};

//////////////////// FE Current Sensor Alert ////////////////////
static ssize_t show_bb_current_temp_sensor_alert(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dvt_priv *priv = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", priv->bb_current_temp_sensor_alert_val);
}

///////////////////// BB Current temp Sensor Alert ///////////////////
static DEVICE_ATTR(bb_current_temp_sensor_alert, S_IRUGO, show_bb_current_temp_sensor_alert, NULL);
///////////////////// END FE Current Sensor Alert  //////////////

static struct attribute *dvt_attrs[] = {
    &dev_attr_bb_current_temp_sensor_alert.attr,
	NULL,
};

static const struct attribute_group dvt_attr_group = {
	.attrs = dvt_attrs,
};

static irqreturn_t bb_curr_temp_handler(int irq, void *dev_id)
{
    printk("Receieving CURENT_TEMP IRQ\n");
    return IRQ_HANDLED;
}

int dvt_parse_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct dvt_priv *priv = dev_get_drvdata(&pdev->dev);

	if (!np) {
		return -EINVAL;
	}

    // bb_current_temp_sensor_alert value
	priv->bb_current_temp_sensor_alert_gpio = of_get_named_gpio(np, "bb-current-temp-sensor-alert-gpio", 0);
	if (priv->bb_current_temp_sensor_alert_gpio < 0) {
		dev_warn(&pdev->dev, "Can't read bb current sensor alert gpio from DT.\n");
	}

	return 0;
}

static int dvt_probe(struct platform_device *pdev)
{
	struct dvt_priv *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct dvt_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	platform_set_drvdata(pdev, priv);

	ret = dvt_parse_dt(pdev);
	if (ret) {	
		return ret;
	}

    printk("bb_current_temp_sensor_alert_gpio number %d\n",  priv->bb_current_temp_sensor_alert_gpio);	
        // BB current temperature sensor alert
        if(priv->bb_current_temp_sensor_alert_gpio > 0)
	{
		sprintf(priv->bb_current_temp_sensor_alert_szGpio, "bb_curr_temp_sensor_alert");
		ret = gpio_request(priv->bb_current_temp_sensor_alert_gpio,
			priv->bb_current_temp_sensor_alert_szGpio);
		if ( ret )
		{
			dev_err(&pdev->dev, "Could not obtain GPIO %d: %s\n",
				priv->bb_current_temp_sensor_alert_gpio, priv->bb_current_temp_sensor_alert_szGpio);
			return ret;
		}
		ret = gpio_direction_input(priv->bb_current_temp_sensor_alert_gpio);
		if ( ret )
		{
			dev_err(&pdev->dev, "Could not configure GPIO %d direction: %d\n",
				priv->bb_current_temp_sensor_alert_gpio, ret);
			return ret;
		}
	}
    #if 01
    //priv->bb_curr_temp_irq_line = gpio_to_irq(priv->bb_current_temp_sensor_alert_gpio);
    priv->bb_curr_temp_irq_line = 25;
    printk("bb_curr_temp_irq_line %d\n", priv->bb_curr_temp_irq_line);

    if(request_irq(priv->bb_curr_temp_irq_line, bb_curr_temp_handler, IRQF_SHARED|IRQ_TYPE_LEVEL_LOW, "dvt_irq", priv )) {
         printk("dvt irq failed\n");
    }
   #endif 
	ret = sysfs_create_group(&priv->dev->kobj, &dvt_attr_group);
	if (ret) {
		dev_err(priv->dev, "unable to create sysfs files\n");
		return ret;
	}
	return 0;
}

static int dvt_remove(struct platform_device *pdev)
{
	struct dvt_priv *priv = platform_get_drvdata(pdev);

	sysfs_remove_group(&priv->dev->kobj, &dvt_attr_group);
	return 0;
}

static const struct of_device_id dvt_of_match[] = {
	{ .compatible = "dvt", },
	{}
};
MODULE_DEVICE_TABLE(of, dvt_of_match);

static struct platform_driver dvt_driver = {
	.driver = {
		.name = "dvt",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dvt_of_match),
	},
	.probe = dvt_probe,
	.remove = dvt_remove,
};

module_platform_driver(dvt_driver);

MODULE_DESCRIPTION("DVT Driver");
MODULE_AUTHOR("<>");
MODULE_LICENSE("GPL");
