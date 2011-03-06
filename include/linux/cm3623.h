
#ifndef LINUX_CM3623_MODULE_H
#define LINUX_CM3623_MODULE_H

/**
 * struct cm3623_platform_data - data to set up cm3623 driver
 *
 * @setup: optional callback to activate the driver.
 * @teardown: optional callback to invalidate the driver.
 *
**/

struct cm3623_platform_data {
    int gpio_int;
	int (*power)(int);    
};

#define CM3623_NAME                     "cm3623"

#endif /* LINUX_CM3623_MODULE_H */
