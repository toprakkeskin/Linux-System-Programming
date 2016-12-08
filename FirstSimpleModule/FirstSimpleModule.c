#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


static int moduleInit(void)
{
	// Say Hello
	printk(KERN_INFO "Hello World!\n");

	return 0; // everything is OK!
}

static void moduleCleanup(void)
{
	// Say Goodbye
	printk(KERN_INFO "Goodbye!\n");
}

MODULE_LICENSE("GPL");
// Register module functions
module_init(moduleInit);
module_exit(moduleCleanup);