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
    int adapter_nr = 0; /* probably dynamically determined */
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
    for (;;) {
        if (read(file, buf, 4) != 4) {
            perror("Failed to read from the i2c bus");
        } else {
            printf("status: %d", buf[0] >> 6);

            printf("humidity: %f\t",
                    (float)( ( (unsigned int) ( (buf[0] << 8) & 0x3F ) | buf[1]) * 100 / (pow(2,14) - 1))
                    );
            printf("temperature: %f\n",
                    (float) (((unsigned int) (buf[2] << 6) + (buf[3] >> 2)) / (pow(2, 14) - 1) * 165 - 40)
                    );
        }
        usleep(3000*1000);//wait
    }
}

