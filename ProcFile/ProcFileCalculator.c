#include <linux/module.h>     // Modül fonksiyonlarını tanımlamak için
#include <linux/kernel.h>	 // Kernel logları için
#include <linux/proc_fs.h>  // ProcFile oluşturabilmek için
#include <linux/slab.h>	   // kmalloc komutu için
#include <linux/string.h> // strlen ve kstrtol komutları için

// ProcFile için kullanacağımız değişkenleri tanımlıyoruz.
size_t procMsgLen,procTempLen;
char* procMsg = NULL;


// Verilen diziyi verilen genişlik kadar
// \0 karakteriyle temizle.
void fillEmpty(char* array, int len){
	int i=0;
	for(; i<=len; i++){
		array[i]='\0';	
	}
}
	
void parseAndCompute(char* array){ 

	// Gelen dizi NULL değil ise devam et.
	if(array != NULL)
	{

		int i=0; // Ayırma işlemi için indis tanımla.
	
		// Bir operatör bulana kadar indis'i arttır.
		while(true){
		        if(array[i] == '/' || array[i] == '*' || array[i] == '-' || array[i] == '+')
					break;
				else
		        	i++;	
		}

		// Soldaki sayi için bir dizi oluştur.
		char leftSide[i+1]; 
		// Bu diziyi boş karakterle doldur. (\0 karakteri)
		fillEmpty(&leftSide,i+1);
	
		// Gelen ana diziden karakterleri soldaki sayının dizisine
		// tek tek kopyala.    
		int tmp;	
		for(tmp=0; tmp<i; tmp++){
			leftSide[tmp]=array[tmp];
		}
	
		// Elde edilen indis bize işlem operatorünü verir.
		char computeOperator=array[i];

		// i son bir kez arttırılır 
		// ve sağdaki sayının başlangıç indisi elde edilir.
		i++;
	
		// Başlangıçta uygulanan döngü son bir kez uygulanır ve birden fazla operatör
		// girilmesi sonucunda, ilk girilen operatör işlem için baz alınır.
		while(true){
	        if(array[i] == '/' || array[i] == '*' || array[i] == '-' || array[i] == '+')
	            i++;
	        else
		       break;	
		}

		// Sağdaki sayı için gerekli genişliği hesapla.
		int rightSideSize=strlen(array)-i-1;

		// Sağdaki sayı için bir dizi oluştur.
		char rightSide[rightSideSize];
		// Bu diziyi boş karakterle doldur. (\0 karakteri)
		fillEmpty(&rightSide, rightSideSize);
		    
		// Gelen ana diziden karakterleri soldaki sayının dizisine
		// tek tek kopyala.   
		int j;
		for(j=0;j<rightSideSize;j++)
		{
			rightSide[j]=array[i];
			i++;
		}
	
		//  Dizileri long tipine dönüştürmek için pointerlar oluştur.
		char *cpLeftSide = leftSide;
		char *cpRightSide = rightSide;
		
		// Dönüştürülen sayılar için değişkenler oluştur.
		long leftNumber,rightNumber;
		 
		// iki sayı içinde dönüştürme işlemi uygula ve işlem gerçekleşirse devam et.
		if(kstrtol(cpLeftSide,10, &leftNumber)==0 && kstrtol(cpRightSide,10, &rightNumber) == 0)
		{
		    
		    int solution;
			
			// İşlem operatörüne göre beklenen çözümü hesapla.
			switch(computeOperator)
			{
				case '+': // işlem operatörü + ise
					solution = leftNumber+rightNumber;
				break;

				case '-': // işlem operatörü - ise
					solution = leftNumber-rightNumber;
				break;

				case '*': // işlem operatörü * ise
					solution = leftNumber*rightNumber;
				break;

				case '/': // işlem operatörü / ise
					if(rightNumber!=0)	// 0'a bölünme durumu kontrolü
						solution = leftNumber/rightNumber;
					else
						solution = -9999;
				break;	
			
				default:
					solution = -9999;  // Beklenmeyen bir operatör dönerse
				break;				  // hata olduğunu gösteren değeri döndür.
							
			}

			// ProcFile'a yazılacak mesajı formatla.
	    	snprintf(procMsg, 30, 
	    			 "%d %c %d = %d\n",
	    			 leftNumber,
	    			 computeOperator,
	    			 rightNumber,
	    			 solution
	    	);
		
			// Dizi genişliğini formatlanan son hale göre düzenle.
			procMsgLen = strlen(procMsg);
		
		} 
		else // Dönüştürme işlemi başarısız olursa log'a mesaj gönder.
		{
			printk(KERN_INFO "Function: parseAndCompute | Donusturme islemi basarisiz. \n ");
		}

	}
	else // Gönderilen dizi boş ise log'a mesaj gönder.
	{
		printk(KERN_INFO "Function: parseAndCompute | Gonderilen dizi bos. \n");
	}
   
}


// ProcFile okuma işlemi için tanımlanan fonksiyon.
int procRead(struct file *filp,char *buf,size_t count,loff_t *offp)
{
	// Eğer okunacak değerin genişliği procTempLen'den
	//  büyükse, genişliği sınırla.
	if(count>procTempLen)
		count=procTempLen;

	// Verinin okunan bölümünün genişliği
	procTempLen=procTempLen-count;
	
	// Varolan mesajı buffer'a kopyala.
	memcpy(buf, procMsg, count);

	// Okunacak bir değer kalmadıysa
	// procTempLen'i proc mesajının genişliğine eşitle.
	if(count==0)
		procTempLen=procMsgLen;
	   
	// Okunan genişliği geri döndür.
	return count;
}

// ProcFile yazma işlemi için tanımlanan fonksiyon.
int procWrite(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	// Dizi genişliği belirlenen maksimum değerden
	// büyük ise genişliği maksimum değere eşitle.
	procMsgLen=count;
	if(procMsgLen > 30)
		procMsgLen = 30;

	// Yazma işlemi yapılmadan önce mesajı temizle.
	fillEmpty(procMsg, 30);
	// Gelen buffer'ı mesaja kopyala.
	memcpy(procMsg, buf, procMsgLen);

	// Parçalama ve hesaplama fonksiyonunu çağır.
	parseAndCompute(procMsg);
	
	// Dizi genişliğini geçici bir değişkende sakla.
	procTempLen=procMsgLen;
	
	// Okunan genişliği geri döndür.
	return procMsgLen;
}

// ProcFile oluşturulduğunda
// Yazma ve Okuma fonksiyonları bu yapıyla
// tanıtılmış olur.
struct file_operations procfOps = {
	read: procRead,
	write: procWrite
};

// Modulün init fonksiyonu.
int proc_init (void) {
	// Değişkenlere başlangıç değerlerini ata.
	procMsgLen=0;
	procTempLen=0;
	// Mesaj için hafızadan maksimum genişlik kadar yer ayır.
	procMsg = kmalloc(sizeof(char)*30,GFP_KERNEL);
	// Eğer mesaj kullanılabilirse procfile oluştur.
	if(procMsg)
		proc_create("calculator",0777,NULL,&procfOps);
	return 0;
}

// Modulün cleanup fonksiyonu.
void proc_cleanup(void) {
	// Procfile temizle.
 	remove_proc_entry("calculator",NULL);
}


// Modulün init fonksiyonunu kayıt ettir.
module_init(proc_init);
// Modulün exit-cleanup fonksiyonunu kayıt ettir.
module_exit(proc_cleanup);


