/*
 * dvt.c - System DVT
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
#define FE_TEMP_ALERT (1 << 1)
#define DVT_MAJOR_NUM 112
 
struct dvt_priv
{
	struct device *dev;
    int bb_current_temp_sensor_alert_gpio;
	char bb_current_temp_sensor_alert_szGpio[32];
    unsigned int bb_curr_temp_irq_line;
    unsigned int bb_current_temp_intr_ctrl;

    int fe_temp_sensor_alert_gpio;
	char fe_temp_sensor_alert_szGpio[32];
    unsigned int fe_temp_irq_line;
    unsigned int fe_temp_intr_ctrl;

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
	count = sprintf(buf, "%d\n", priv->bb_current_temp_intr_ctrl);
    spin_unlock_irq(&priv->lock);
    return count;
}

static ssize_t show_fe_temp_alert_enable(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct dvt_priv *priv = dev_get_drvdata(dev);
    int count = 0;
    spin_lock_irq(&priv->lock);
    count = sprintf(buf, "%d\n", priv->fe_temp_intr_ctrl);
    spin_unlock_irq(&priv->lock);
    return count;
}

static ssize_t show_dvt_alert_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dvt_priv *priv = dev_get_drvdata(dev);
    int count = 0;
    spin_lock_irq(&priv->lock);
	count = sprintf(buf, "%u\n", priv->irq_status);
    spin_unlock_irq(&priv->lock);
    return count;
}
 
/* Application is supposed to clear the irq status */
static ssize_t set_dvt_alert_status(struct device *dev,
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
    if((ctrl == ENABLE)&& (priv->bb_current_temp_intr_ctrl == DISABLE)) {

        // Enable IRQ
        priv->bb_current_temp_intr_ctrl = ENABLE;
        spin_unlock_irq(&priv->lock);
        enable_irq(priv->bb_curr_temp_irq_line);

    }else if((ctrl == DISABLE) && (priv->bb_current_temp_intr_ctrl == ENABLE)){ 
        spin_unlock_irq(&priv->lock);

         // DISABLE IRQ
         disable_irq(priv->bb_curr_temp_irq_line);
         priv->bb_current_temp_intr_ctrl = DISABLE;
    }else{

        spin_unlock_irq(&priv->lock);
       //Do nothing
    }
    return count;
}

static ssize_t set_fe_temp_alert_enable(struct device *dev,
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
    if((ctrl == ENABLE)&& (priv->fe_temp_intr_ctrl == DISABLE)) {
        // Enable IRQ
        priv->fe_temp_intr_ctrl = ENABLE;
        spin_unlock_irq(&priv->lock);
        enable_irq(priv->fe_temp_irq_line);

    }else if((ctrl == DISABLE) && (priv->fe_temp_intr_ctrl == ENABLE)){
         spin_unlock_irq(&priv->lock);
         // DISABLE IRQ
         disable_irq(priv->fe_temp_irq_line);
         priv->fe_temp_intr_ctrl = DISABLE;
    }else{

        spin_unlock_irq(&priv->lock);
       //Do nothing
    }
    return count;
}


///////////////////// BB Current temp Sensor Alert ///////////////////
static DEVICE_ATTR(bb_current_temp_alert_enable, S_IWUSR|S_IRUGO, show_bb_current_temp_alert_enable, set_bb_current_temp_alert_enable);
static DEVICE_ATTR(fe_temp_alert_enable, S_IWUSR|S_IRUGO, show_fe_temp_alert_enable, set_fe_temp_alert_enable);
static DEVICE_ATTR(dvt_alert_status, S_IWUSR|S_IRUGO, show_dvt_alert_status, set_dvt_alert_status);
///////////////////// END FE Current Sensor Alert  //////////////

static struct attribute *dvt_attrs[] = {
    &dev_attr_bb_current_temp_alert_enable.attr,
    &dev_attr_fe_temp_alert_enable.attr,
    &dev_attr_dvt_alert_status.attr,
	NULL,
};

static const struct attribute_group dvt_attr_group = {
	.attrs = dvt_attrs,
};

static irqreturn_t bb_curr_temp_handler(int irq, void *dev_id)
{
    printk(KERN_DEBUG "DVT: Receiving BB_CURRENT_TEMP IRQ_\n");
    gpriv->irq_status |= BB_CURR_TEMP_ALERT;
    wake_up_interruptible(&irq_wait);
    disable_irq_nosync(gpriv->bb_curr_temp_irq_line);
    gpriv->bb_current_temp_intr_ctrl = DISABLE;
    return IRQ_HANDLED;
}

static irqreturn_t fe_temp_handler(int irq, void *dev_id)
{
    printk(KERN_DEBUG "DVT: Receiving FE_TEMP IRQ\n");
    gpriv->irq_status |= FE_TEMP_ALERT;
    wake_up_interruptible(&irq_wait);
    disable_irq_nosync(gpriv->fe_temp_irq_line);
    gpriv->fe_temp_intr_ctrl = DISABLE;
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
		dev_warn(&pdev->dev, "DVT: Can't read bb current sensor alert gpio from DVT.\n");
	}

	priv->fe_temp_sensor_alert_gpio = of_get_named_gpio(np, "fe-temp-sensor-alert-gpio", 0);
	if (priv->fe_temp_sensor_alert_gpio < 0) {
		dev_warn(&pdev->dev, "DVT: Can't read fe_temp_sensor_alert_gpio alert gpio from DVT.\n");
	}

	return 0;
}

static unsigned int dvt_setup_gpio_irq(char *name, struct platform_device *pdev, int gpio, char *szGpio)
{
    int ret;

    printk(KERN_DEBUG "DVT: gpio number %d\n", gpio);
    if(gpio > 0)
    {
       sprintf(szGpio, name);
       ret = gpio_request(gpio, szGpio);
       if ( ret )
       {
           dev_err(&pdev->dev, "DVT: Could not obtain GPIO %d: %s\n", gpio, szGpio);
           return ret;
       }
       ret = gpio_direction_input(gpio);
       if ( ret )
       {
           dev_err(&pdev->dev, "DVT: Could not configure GPIO %d direction: %d\n", gpio, ret);
           return ret;
       }
   }

   return octeon_gpio_irq(gpio);
}

static int dvt_probe(struct platform_device *pdev)
{
	struct dvt_priv *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct dvt_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

    gpriv = priv;
    priv->bb_current_temp_intr_ctrl = DISABLE;
    priv->fe_temp_intr_ctrl = DISABLE;
    priv->irq_status = 0;
	priv->dev = &pdev->dev;
	platform_set_drvdata(pdev, priv);

	ret = dvt_parse_dt(pdev);
	if (ret) {	
		return ret;
	}

    spin_lock_init(&priv->lock);

    // BB intr
    priv->bb_curr_temp_irq_line = dvt_setup_gpio_irq("bb_curr_temp_alert", pdev,
        priv->bb_current_temp_sensor_alert_gpio, priv->bb_current_temp_sensor_alert_szGpio);


    if(request_irq(priv->bb_curr_temp_irq_line, bb_curr_temp_handler, IRQF_SHARED|IRQ_TYPE_LEVEL_LOW, "bb_curr_temp_irq", priv )) {
         printk(KERN_DEBUG "bb_curr_temp_irq\n");
         return -1;
    }

    /* Since the default of current alert setting starts at zero; 
       there is an inter imediately after request_irq which creates
       race condition hence need to check the below.
    */
    spin_lock_irq(&priv->lock);
    if(priv->bb_current_temp_intr_ctrl == ENABLE)
    {
        disable_irq(priv->bb_curr_temp_irq_line);
        priv->bb_current_temp_intr_ctrl = DISABLE; 
    }
    spin_unlock_irq(&priv->lock);

    // FE intr
     priv->fe_temp_irq_line = dvt_setup_gpio_irq("fe_temp_alert", pdev, 
           priv->fe_temp_sensor_alert_gpio, priv->fe_temp_sensor_alert_szGpio);

     if(request_irq(priv->fe_temp_irq_line, fe_temp_handler, IRQF_SHARED|IRQ_TYPE_LEVEL_LOW, "fe_temp_irq", priv )) {
         printk(KERN_DEBUG "fe_temp_irq\n");
         return -1;
    }

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
