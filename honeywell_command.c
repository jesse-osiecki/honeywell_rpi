#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>



////////////////
//  main()
////////////////
int main(int argc, char *argv[]) {
    int file;
    int adapter_nr = 1; /* probably dynamically determined */
    char filename[20];
    int addr = 0x27; /* The I2C address */
    char buf[10];


    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if (file < 0) {
        perror("Fuck opening the dev file");
        exit(1);
    }

    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("Fuck setting the slave address < 0");
        exit(1);
    }

    //3ms window before programming
    usleep(3000);

    //send START_CM command
    buf[0] = 0xA0;
    buf[1] = 0x00;
    buf[2] = 0x00;
    while(write(file, buf, 3) != 3){
        perror("Failed to write to i2c device ");
    }
    
if (read(file, buf, 1) != 1) {
    perror("Failed to read from the i2c bus");
} else {
    printf("status: %04x\n", buf[0] >> 6);
    printf("diagnostic: %04x\n", (buf[0] >> 2) & 0x0f );
    printf("response: %04x\n", (buf[0] ) & 0x03 );

}
usleep(3000*100);//wait
return 0;
}
