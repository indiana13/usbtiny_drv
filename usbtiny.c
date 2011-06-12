#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>

#include "controls.h"

#define USBTINY_DEV_NAME	"USBTinyISP"
#define USBTINY_VENDOR_ID	0x1781
#define USBTINY_PRODUCT_ID	0x0C9F

static struct usb_device_id usbtiny_table[] = {
	{ USB_DEVICE(USBTINY_VENDOR_ID, USBTINY_PRODUCT_ID) },
	{ }					
};
MODULE_DEVICE_TABLE(usb, usbtiny_table);

#define USBTINY_MINOR_BASE	192

struct usbtiny_isp {
	struct kref refcount;
};

#define to_usbtiny_dev(d) container_of(d, struct usbtiny_isp, refcount)

static const struct file_operations usbtiny_fops = {
	.owner =	THIS_MODULE,
	0
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver usbtiny_class = {
	.name =		"usbtiny%d",
	.minor_base =	USBTINY_MINOR_BASE,
	.fops = 	&usbtiny_fops,
};

static void usbtiny_delete(struct kref *kref)
{
	struct usbtiny_isp *dev = to_usbtiny_dev(kref);
	kfree(dev);
}

static int usbtiny_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	int retval = -ENOMEM;
	struct usbtiny_isp *dev;
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if(!dev) {
		err("Out of memory");
		goto error;
	}
	kref_init(&dev->refcount);
	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);
	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &usbtiny_class);
	if (retval) {
		/* something prevented us from registering this driver */
		err("Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}
	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "USBTinyISP device now attached to /dev/usbtiny%d",
		 interface->minor);
	/* TODO: send POWER_ON control URB */
	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->refcount, usbtiny_delete);
	return retval;
}

static void usbtiny_disconnect(struct usb_interface *interface)
{
	struct usbtiny_isp *dev;
	int minor = interface->minor;
	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);
	/* give back our minor */
	usb_deregister_dev(interface, &usbtiny_class);
	/* this frees allocated memory */
	kref_put(&dev->refcount, usbtiny_delete);
	dev_info(&interface->dev, 
		 "USBTinyISP #%d now disconnected", 
		 minor);
}

static struct usb_driver usbtiny_driver = {
	.name =		USBTINY_DEV_NAME,
	.probe =	usbtiny_probe,
	.disconnect =	usbtiny_disconnect,
	.id_table =	usbtiny_table,
	{}
};

static int __init usbtiny_init(void)
{
	int result;
	result = usb_register(&usbtiny_driver);
	if (result)
		err("USBTinyISP driver failed to register (%d)", result);
	return result;
}

static void __exit usbtiny_exit(void)
{
	usb_deregister(&usbtiny_driver);
}

module_init(usbtiny_init);
module_exit(usbtiny_exit);

MODULE_LICENSE("GPL");
