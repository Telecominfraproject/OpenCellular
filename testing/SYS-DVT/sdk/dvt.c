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
 #include <linux/kernel.h>
 #include <linux/types.h>
 #include <linux/platform_device.h>
 #include <linux/io.h>
 #include <linux/of_gpio.h>
 #include <linux/of.h>
 #include <linux/sysfs.h>
 #include <linux/spinlock.h>
 #include <linux/poll.h>
 #include <linux/wait.h>
 #include <linux/sched.h>

unsigned int octeon_gpio_irq(int);
static DECLARE_WAIT_QUEUE_HEAD(irq_wait);
static struct dvt_priv *gpriv;

#define DISABLE 0
#define ENABLE 1
#define BB_CURR_TEMP_ALERT (1 << 0)
#define DVT_MAJOR_NUM 112
 
struct dvt_priv
{
	struct device *dev;
    int bb_current_temp_sensor_alert_gpio;
	char bb_current_temp_sensor_alert_szGpio[32];
	u32 bb_current_temp_sensor_alert_default;

    unsigned int bb_curr_temp_irq_line;

    unsigned int intr_flag;
    unsigned int irq_status;
    spinlock_t lock;    
};


//////////////////// FE Current Sensor Alert ////////////////////
static ssize_t show_bb_current_temp_alert_enable(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dvt_priv *priv = dev_get_drvdata(dev);
    int count = 0;
    spin_lock_irq(&priv->lock);
	count = sprintf(buf, "%d\n", priv->intr_flag);
    spin_unlock_irq(&priv->lock);
    return count;
}

static ssize_t show_bb_current_temp_alert_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dvt_priv *priv = dev_get_drvdata(dev);
    int count = 0;
    spin_lock_irq(&priv->lock);
	count = sprintf(buf, "%u\n", priv->irq_status);
    spin_unlock_irq(&priv->lock);
    return count;
} 

static ssize_t set_bb_current_temp_alert_status(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct dvt_priv *priv = dev_get_drvdata(dev);
    u32 ctrl;
    int ret;

    ret = kstrtouint(buf, 0, &ctrl);
    if(ret)
        return ret;

    spin_lock_irq(&priv->lock);
    priv->irq_status = ctrl;
    spin_unlock_irq(&priv->lock);

    return count;
}

static ssize_t set_bb_current_temp_alert_enable(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
	struct dvt_priv *priv = dev_get_drvdata(dev);
    u32 ctrl;
    int ret;

    ret = kstrtouint(buf, 0, &ctrl);
    if(ret)
        return ret;
    
    spin_lock_irq(&priv->lock);
    if((ctrl == ENABLE)&& (priv->intr_flag == DISABLE)) {

        // Enable IRQ
        priv->intr_flag = ENABLE;
        spin_unlock_irq(&priv->lock);
        enable_irq(priv->bb_curr_temp_irq_line);

    }else if((ctrl == DISABLE) && (priv->intr_flag == ENABLE)){ 
        spin_unlock_irq(&priv->lock);

         // DISABLE IRQ
         disable_irq(priv->bb_curr_temp_irq_line);
         priv->intr_flag = DISABLE;
    }else{

        spin_unlock_irq(&priv->lock);
       //Do nothing
    }
    return count;
}
///////////////////// BB Current temp Sensor Alert ///////////////////
static DEVICE_ATTR(bb_current_temp_alert_enable, S_IWUSR|S_IRUGO, show_bb_current_temp_alert_enable, set_bb_current_temp_alert_enable);
static DEVICE_ATTR(bb_current_temp_alert_status, S_IWUSR|S_IRUGO, show_bb_current_temp_alert_status, set_bb_current_temp_alert_status);
///////////////////// END FE Current Sensor Alert  //////////////

static struct attribute *dvt_attrs[] = {
    &dev_attr_bb_current_temp_alert_enable.attr,
    &dev_attr_bb_current_temp_alert_status.attr,
	NULL,
};

static const struct attribute_group dvt_attr_group = {
	.attrs = dvt_attrs,
};

static irqreturn_t bb_curr_temp_handler(int irq, void *dev_id)
{
    printk(KERN_DEBUG "Receieving CURENT_TEMP IRQ\n");
    gpriv->irq_status |= BB_CURR_TEMP_ALERT;
    wake_up_interruptible(&irq_wait);
    disable_irq_nosync(gpriv->bb_curr_temp_irq_line);
    gpriv->intr_flag = DISABLE;
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

    gpriv = priv;
    priv->intr_flag = DISABLE;
    priv->irq_status = 0;
	priv->dev = &pdev->dev;
	platform_set_drvdata(pdev, priv);

	ret = dvt_parse_dt(pdev);
	if (ret) {	
		return ret;
	}

    spin_lock_init(&priv->lock);
    printk(KERN_DEBUG "bb_current_temp_sensor_alert_gpio number %d\n",  priv->bb_current_temp_sensor_alert_gpio);	
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

    priv->bb_curr_temp_irq_line = octeon_gpio_irq(priv->bb_current_temp_sensor_alert_gpio);
    printk(KERN_DEBUG "bb_curr_temp_irq_line %d\n", priv->bb_curr_temp_irq_line);

    //if(request_irq(priv->bb_curr_temp_irq_line, bb_curr_temp_handler, IRQ_TYPE_EDGE_FALLING, "dvt_irq", priv )) {
    if(request_irq(priv->bb_curr_temp_irq_line, bb_curr_temp_handler, IRQF_SHARED|IRQ_TYPE_LEVEL_LOW, "dvt_irq", priv )) {
         printk(KERN_DEBUG "dvt irq failed\n");
         gpio_free(priv->bb_current_temp_sensor_alert_gpio);
         devm_kfree(&pdev->dev, priv);
         return -1;
    }

    spin_lock_irq(&priv->lock);
    priv->intr_flag = ENABLE; 
    spin_unlock_irq(&priv->lock);

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

static unsigned int dvt_poll(struct file *fp, poll_table *w)
{
    unsigned int d;

    poll_wait(fp, &irq_wait, w);

    spin_lock_irq(&gpriv->lock);
    d = gpriv->irq_status;
    spin_unlock_irq(&gpriv->lock);

    if(d) {
        // IRQ will be disabled once the first int is received;
        // Since irq clearing will happen in the application we clear the irq_status
        gpriv->irq_status = 0;
        return POLLIN | POLLRDNORM;
    }
   
    return 0;
}

static int dvt_open(struct inode *inode, struct file *fp)
{
    //do nothing
    return 0;
}

static int dvt_release(struct inode *inode, struct file *fp)
{
    // do nothing
    return 0;
}

struct file_operations dvt_fops = {

    .open = dvt_open,
    .release = dvt_release,
    .poll = dvt_poll,
};

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

static int __init dvt_init(void)
{
    if(register_chrdev(DVT_MAJOR_NUM, "dvt", &dvt_fops)) {
       printk(KERN_DEBUG "DVT driver registeration failed\n");
    }
    platform_driver_register(&dvt_driver);
    return 0;
}

static void __exit dvt_exit(void)
{
    platform_driver_unregister(&dvt_driver);
    unregister_chrdev(DVT_MAJOR_NUM, "dvt");
}
//module_platform_driver(dvt_driver);

module_init(dvt_init);
module_exit(dvt_exit);

MODULE_DESCRIPTION("DVT Driver");
MODULE_AUTHOR("<>");
MODULE_LICENSE("GPL");
