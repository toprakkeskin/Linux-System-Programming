#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/// Device Definitions
#define DEVICE_NAME "raspLedC"
#define IOCTL_SET_MSG _IOR(95, 0, char *)
static char op = '\0';
static unsigned int deviceIsOpened = 0;

/// Device Functions
void deviceRegistry(int major, char deviceName[]);
static int devOpen(struct inode *, struct file *);
static int devRelease(struct inode *, struct file *);
static ssize_t devWrite(struct file *, const char *, size_t, loff_t *);
long devIoctl(struct file *, unsigned int, unsigned long);

void stateInterpreter(char);


/// GPIO: Leds
static struct gpio leds[] = {
	{ 15, GPIOF_OUT_INIT_HIGH, "LED 1" },
	{ 18, GPIOF_OUT_INIT_HIGH, "LED 2" },
	{ 23, GPIOF_OUT_INIT_HIGH, "LED 3" },
	{ 24, GPIOF_OUT_INIT_HIGH, "LED 4" },
	{ 25, GPIOF_OUT_INIT_HIGH, "LED 5" }
};

/// GPIO: Buzzers
static struct gpio buzzers[] = {
	{ 8, GPIOF_OUT_INIT_LOW, "BUZZER 1" }
};

// Timer Definitions
static struct timer_list blink_timer;
static struct timer_list seqTimer;
static struct timer_list buzzerTimer;


/// GPIO Control Functions
void ledControl(int, unsigned int, unsigned int);
void beep(unsigned long time);

/// Timer Functions
static void blinkTimerFunc(unsigned long);
static void seqTimerFunc(unsigned long);
static void buzzerTimerFunc(unsigned long);
void delLedTimers(void);
void delBuzzerTimers(void);
void delAllTimers(void);


// Structure containing callbacks
static struct file_operations fops =
{
	.open = devOpen, //address data
	.write = devWrite, //addres data
	.unlocked_ioctl = devIoctl,
	.release = devRelease //address data

};

#endif