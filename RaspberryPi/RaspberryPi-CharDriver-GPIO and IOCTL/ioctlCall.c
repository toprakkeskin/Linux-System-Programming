#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#define IOCTL_SET_MSG _IOR(95,0,char*)

int main(int argc, char **argv)
{
	/* Gönderilen parametre sayısını kontrol et. */
  if(argc > 1)
  {
    int file;
    char* temp = argv[1];
	/* Aygıtı aç. */
	file = open("/dev/raspLedC", O_RDONLY);
	/* IOCTL çağrısı yap, mesajı ilet. */
    if (ioctl(file, IOCTL_SET_MSG, temp) == -1){
       printf("IOCTL_SET_MSG failed: %s\n",
              strerror(errno));
    }
   close(file);
  }
 return 0;
}


