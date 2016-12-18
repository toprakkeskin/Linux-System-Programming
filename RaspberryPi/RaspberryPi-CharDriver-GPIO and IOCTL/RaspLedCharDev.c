#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/delay.h>
#include "Definitions.h" // Gerekli tüm tanımlamaları barındırır


/*
	Modül yüklenirken çalışır
*/
int init_module(void)
{
	int ret = 0;
	
	/// Kullanılacak ledlerin kontrolünü ister
	ret = gpio_request_array(leds, ARRAY_SIZE(leds));
	if (ret) {
		printk(KERN_ERR "Unable to request Led's GPIOs");
		return ret;
	}

	/// Kullanılacak buzzerların kontrolünü ister
	ret = gpio_request_array(buzzers, ARRAY_SIZE(buzzers));
	if (ret) {
		printk(KERN_ERR "Unable to request Buzzer's GPIOs");
		return ret;
	}
	beep(4); // 1/4 saniye
	deviceRegistry(95, DEVICE_NAME); // Aygıtı sisteme kaydet.

	return ret;
}

/*
	Modül silinirken çalışır.
*/
void cleanup_module(void)
{
	beep(4);
	msleep(250); // Son beep için kapanmadan önce 250ms bekle
	delAllTimers();	// Tüm sayaçları sil.

	/// Tüm ledleri söndür.
	int i;
	for (i = 0; i < ARRAY_SIZE(leds); i++)
	{
		gpio_set_value(leds[i].gpio, 0);
	}

	/// Tüm buzzerları kapat.
	for (i = 0; i < ARRAY_SIZE(buzzers); i++)
	{
		gpio_set_value(buzzers[i].gpio, 0);
	}

	gpio_free_array(leds, ARRAY_SIZE(leds)); // Kitlenilen ledleri serbest bırak.
	gpio_free_array(buzzers, ARRAY_SIZE(buzzers)); // Kitlenilen buzzerları serbest bırak.
	unregister_chrdev(95, DEVICE_NAME); // Aygıtı sistemden sil.
}

/*
	Bir aygıtı sisteme kayıt ettirir
*/
void deviceRegistry(int major, char deviceName[])
{
	int t = register_chrdev(major, deviceName, &fops);
	if (t<0)
		printk(KERN_ALERT "Device: %s, Registration Failed...\n", DEVICE_NAME);
	else
		printk(KERN_ALERT "Device: %s, Registered...\n", DEVICE_NAME);
}

/*
	Aygıt bir işlem için her çalıştırıldığında çalışır
*/
static int devOpen(struct inode *inod, struct file *fil)
{
	/*
	  Aygıt eğer başka bir kullanıcı tarafından şuanda kullanılıyorsa
	  ikinci bir kullanıcıya aygıta erişim izni vermez.
	*/
	if (deviceIsOpened) return -EBUSY;
	deviceIsOpened++;

	return 0;
}

/*
	Aygıtın yaptığı işlemler bittikten sonra çalışır
	(Aygıtın serbest kalması durumu)
*/
static int devRelease(struct inode *inod, struct file *fil)
{
	printk(KERN_ALERT"Device released\n");
	
	deviceIsOpened--; // Aygıtı yeniden açılabilir kıl
	op = '\0'; 	// İşlem operatörünü sıfırla

	return 0;
}

/*
	Aygıta veri yazılacağı zaman çalışan fonksiyon
*/
static ssize_t devWrite(struct file *filp, const char *buff, size_t len, loff_t *off)
{

	short count = 0;
	
	//	Aygıt'a gönderilen veri karakterlerine ayrılır.
	while (len>0)
	{
		// Şuanki karakteri işleme al.
		stateInterpreter(buff[count]);
		count++; // Sonraki karaktere ilerle	
		len--; // Okunan karakter sayısını arttır
	}
	return count;
}



/*
	IOCTL sistem çağrısının isteklerini karşılar
*/
long devIoctl(struct file *file,	
	unsigned int ioctl_num,	/* ioctl çağrısının işlem tipi */
	unsigned long ioctl_param) // ioctl çağrısı üzerinden gelen bilgi.
{							  // Gönderilen veri tipine dönüştürülebilmektedir. 
			
	int i;
	char *temp;
	
	switch (ioctl_num) {
		case IOCTL_SET_MSG:
			temp = (char *)ioctl_param;
			size_t tempSize = strlen(temp);
			int c = 0;
			/* 
				ioctl çağrısı geldiğinden gelen parametreyi kontrol eder ve boyutu 0'dan 
				büyükse gelen değeri modüldeki komut yorumlayıcısına aktarır 
			*/
			if (tempSize > 0) {
				while (temp[c] != '\0')
				{
					stateInterpreter(temp[c]);
					if (c + 1 <= tempSize)
						c++;
					else
						break;
				}
			}
			break;
	}
	return 0;
}


/* 
	Led çakması zamanlayıcı kontrolü
*/
static void blinkTimerFunc(unsigned long data)
{
	beep(8); // 1/8 saniye
	int a;
	for (a = 0; a < ARRAY_SIZE(leds); a++) {
		ledControl(1, a, data);
	}

	/* Zamanlayıcı yeniden ayarla */
	blink_timer.data = !data; // Ledin değerini aksine çevir.
	blink_timer.expires = jiffies + (1 * HZ)/4; // 0.25 saniye.
	add_timer(&blink_timer);
}

/*
	Sıralı led animasyonu zamanlayıcı kontrolü
*/
static void seqTimerFunc(unsigned long data)
{

	beep(8); // 1/8 saniye

	int b;
	ledControl(1, data, 1);

	for (b = 0; b < ARRAY_SIZE(leds); b++) {
		if (b != data)
			ledControl(1, b, 0);
	}

	/* Zamanlayıyıcı ayarla, bilgiyi yükle,
	bitiş süresini belirle ve başlat */

	if ((data + 1) <= 4)
		seqTimer.data = data + 1;
	else
		seqTimer.data = 0;

	seqTimer.expires = jiffies + (1 * HZ)/4; 		// 0.25 saniye
	add_timer(&seqTimer);
}

/*
	Buzzer zamanlayıcı kontrolü
*/
static void buzzerTimerFunc(unsigned long data)
{
	if (data == 0) {
		gpio_set_value(buzzers[0].gpio, 0);
	}
	else {
		gpio_set_value(buzzers[0].gpio, 1);
		/* Zamanlayıyıcı ayarla, bilgiyi yükle,
		bitiş süresini belirle ve başlat */
		buzzerTimer.data = 0L;
		buzzerTimer.expires = jiffies + (1 * HZ) / data;	// 1 saniye
		add_timer(&buzzerTimer);
	}
	
}


/*
	Ledlere toplu veya tek tek müdahale eder
*/
void ledControl(int isSingle, unsigned int index, unsigned int data) {
	if (isSingle==0) {
		int c;
		for (c = 0; c < ARRAY_SIZE(leds); c++) {
			gpio_set_value(leds[c].gpio, data);
		}
	}
	else {
		gpio_set_value(leds[index].gpio, data);
	}
	

}

/*
	Buzzer'ın ötmesini sağlar
*/
void beep(unsigned long time)
{
	if (time != 0)
		beep(0);

	delBuzzerTimers();
	/* Zamanlayıyıcı ayarla, bilgiyi yükle,
		bitiş süresini belirle ve başlat */
	init_timer(&buzzerTimer);
	buzzerTimer.function = buzzerTimerFunc;
	buzzerTimer.data = time;
	buzzerTimer.expires = jiffies + (1 * HZ) / 100;	// 0.25 saniye
	add_timer(&buzzerTimer);
}

/* Led zamanlayıcılarını siler */
void delLedTimers()
{
	del_timer_sync(&blink_timer);
	del_timer_sync(&seqTimer);
}

/* Buzzer zamanlayıcılarını siler */
void delBuzzerTimers() 
{
	del_timer_sync(&buzzerTimer);
}

/* Tüm zamanlayıcıları siler */
void delAllTimers()
{
	delBuzzerTimers();
	delLedTimers();
}

/* Gönderilen komutları yorumlar */
void stateInterpreter(char ch) {
	/* Eğer işlem operatörü boşsa ve gelen veri uygunsa
		gelen veriyi işlem operatörü olarak ata.*/
	if (op == '\0' && (ch == '+' || ch == '-'))
		op = ch;
	else
	{	/* İşlem operatörü + veya - ise devam et.  */
		if (op == '+' || op == '-')
		{
			/* İşleme alınan karakter belirlenen modele uygunsa zamanlayıcıları sil 
				ve buzzer'ı sustur.
				*/
			if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5'
				|| ch == 'a' || ch == 'b' || ch == 's') {
				delLedTimers();
				beep(0);
			}

			switch (ch)
			{
			case '1':
				if (op == '+') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[1] turned on!\n", DEVICE_NAME);
					ledControl(1, 0, 1);
				}
				else if (op == '-') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[1] turned off!\n", DEVICE_NAME);
					ledControl(1, 0, 0);
				}
				break;

			case '2':
				if (op == '+') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[2] turned on!\n", DEVICE_NAME);
					ledControl(1, 1, 1);
				}
				else if (op == '-') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[2] turned off!\n", DEVICE_NAME);
					ledControl(1, 1, 0);
				}
				break;

			case '3':
				if (op == '+') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[3] turned on!\n", DEVICE_NAME);
					ledControl(1, 2, 1);
				}
				else if (op == '-') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[3] turned off!\n", DEVICE_NAME);
					ledControl(1, 2, 0);
				}
				break;

			case '4':
				if (op == '+') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[4] turned on!\n", DEVICE_NAME);
					ledControl(1, 3, 1);
				}
				else if (op == '-') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[4] turned off!\n", DEVICE_NAME);
					ledControl(1, 3, 0);
				}
				break;

			case '5':
				if (op == '+') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[5] turned on!\n", DEVICE_NAME);
					ledControl(1, 4, 1);
				}
				else if (op == '-') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, Led[5] turned off!\n", DEVICE_NAME);
					ledControl(1, 4, 0);
				}
				break;

			case 'a':
				if (op == '+') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, all leds turned on!\n", DEVICE_NAME);
					ledControl(0, 0, 1);
				}
				else if (op == '-') {
					beep(4); // 1/4 saniye
					printk(KERN_ALERT "Device: %s, all leds turned off!\n", DEVICE_NAME);
					ledControl(0, 0, 0);


				}
				break;
			case 'b':
				if (op == '+') {
					del_timer_sync(&seqTimer);
					/* Zamanlayıyıcı ayarla, bilgiyi yükle,
					bitiş süresini belirle ve başlat */
					init_timer(&blink_timer);
					blink_timer.function = blinkTimerFunc;
					blink_timer.data = 1L;							
					blink_timer.expires = jiffies + (1 * HZ) / 4; 		// 0.25 saniye
					add_timer(&blink_timer);

					printk(KERN_ALERT "Device: %s, Blink started!\n", DEVICE_NAME);
				}
				break;

			case 's':
				if (op == '+') {
					del_timer_sync(&blink_timer);
					/* Zamanlayıyıcı ayarla, bilgiyi yükle,
					bitiş süresini belirle ve başlat */
					init_timer(&seqTimer);

					seqTimer.function = seqTimerFunc;
					seqTimer.data = 0L;							
					seqTimer.expires = jiffies + (1 * HZ) / 4; 		// 0.25 saniye
					add_timer(&seqTimer);
					printk(KERN_ALERT "Device: %s, Animation started!\n", DEVICE_NAME);
				}
				break;
			}
		}	
	}
}


// Module Attributes
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A CharDriver that control RaspberryPi's GPIO.It's also includes ioctl. ");
