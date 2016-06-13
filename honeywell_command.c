#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
 
#define IN  0
#define OUT 1
 
#define LOW  0
#define HIGH 1
 
#define POUT 4  /* P1-07 */
#define BUFFER_MAX 3 //for GPIO
#define DIRECTION_MAX 35 //for GPIO
#define VALUE_MAX 30 //for GPIO
 
static int GPIOExport(int pin){
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
 
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}
 
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}
 
//static int GPIOUnexport(int pin){
//	char buffer[BUFFER_MAX];
//	ssize_t bytes_written;
//	int fd;
// 
//	fd = open("/sys/class/gpio/unexport", O_WRONLY);
//	if (-1 == fd) {
//		fprintf(stderr, "Failed to open unexport for writing!\n");
//		return(-1);
//	}
// 
//	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
//	write(fd, buffer, bytes_written);
//	close(fd);
//	return(0);
//}
 
static int GPIODirection(int pin, int dir){
	static const char s_directions_str[]  = "in\0out";
 
	char path[DIRECTION_MAX];
	int fd;
 
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}
 
	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}
 
	close(fd);
	return(0);
}
 
//static int GPIORead(int pin){
//	char path[VALUE_MAX];
//	char value_str[3];
//	int fd;
// 
//	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
//	fd = open(path, O_RDONLY);
//	if (-1 == fd) {
//		fprintf(stderr, "Failed to open gpio value for reading!\n");
//		return(-1);
//	}
// 
//	if (-1 == read(fd, value_str, 3)) {
//		fprintf(stderr, "Failed to read value!\n");
//		return(-1);
//	}
// 
//	close(fd);
// 
//	return(atoi(value_str));
//}
 
static int GPIOWrite(int pin, int value){
	static const char s_values_str[] = "01";
 
	char path[VALUE_MAX];
	int fd;
 
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}
 
	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}
 
	close(fd);
	return(0);
}

static int toggle_gpio(){
    if (-1 == GPIOWrite(POUT, LOW))
        return(3);
    usleep(1000);
    if (-1 == GPIOWrite(POUT, HIGH))
        return(3);
}

////////////////
//  main()
////////////////
int main(int argc, char *argv[]) {
    int file;
    int adapter_nr = 1; /* probably dynamically determined */
    char filename[20];
    int addr = 0x27; /* The I2C address can be as low as 00 or as high as 7f*/
    char buf[10];
    int new_addr = 0x44;

	/*
	 * Enable GPIO pins
	 */
	if (-1 == GPIOExport(POUT))
		return(1);
 
	/*
	 * Set GPIO directions
	 */
	if (-1 == GPIODirection(POUT, OUT))
		return(2);

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
    /*
     * Toggle GPIO to reset the I2C device
     */
    toggle_gpio();
    //We now have 3ms to enter command mode
    //send START_CM command
    buf[0] = 0xA0;
    buf[1] = 0x00;
    buf[2] = 0x00;
    while(write(file, buf, 3) != 3){
        perror("Waiting....");
    }

    //10ms window before programming
    usleep(10000);
    if (read(file, buf, 1) != 1) {
        perror("Failed to read from the i2c bus");
    } else {
        printf("raw: 0x%02x\n", buf[0]);
        printf("status: 0x%02x\n", buf[0] >> 6);
        printf("diagnostic: 0x%02x\n", (buf[0] >> 2) & 0x0f );
        printf("response: 0x%02x\n", (buf[0] ) & 0x03 );

    }
    if( (buf[0] & 0x03) == 0x01 ) { //response is good
        //get EEPROM contents
        buf[0] = 0x1c;
        buf[1] = 0x00;
        buf[2] = 0x00;
        while(write(file, buf, 3) != 3){
            perror("Failed to write to i2c device ");
            exit(1);
        }
        //Perform data fetch
        if (read(file, buf, 3) != 3) {
            perror("Failed to read from the i2c bus");
            exit(1);
        }
        else{
            printf("Data Fetch Customer register: 0x%01x 0x%01x 0x%01x\n", buf[0], buf[1], buf[2]);
            printf("Reported Address is: 0x%01x\n", (buf[2] & 0x7f));
        }
        //now set the new address
        buf[0] = 0x5c;
        buf[2] = buf[2] & 0x80; // make sure we keep the MSbit if it is set for an alarm or something stupid (it's the right thing to do)
        buf[2] = buf[2] | new_addr;
        if(write(file, buf, 3) != 3){
            perror("Failed to write to i2c device ");
            exit(1);
        }
        //We need to wait 12ms for the write to work
        usleep(13000);
        //get EEPROM contents
        buf[0] = 0x1c;
        buf[1] = 0x00;
        buf[2] = 0x00;
        while(write(file, buf, 3) != 3){
            perror("Failed to write to i2c device ");
            exit(1);
        }
        //Perform data fetch
        if (read(file, buf, 3) != 3) {
            perror("Failed to read from the i2c bus");
            exit(1);
        }
        else{
            printf("Data Fetch Customer register: 0x%01x 0x%01x 0x%01x\n", buf[0], buf[1], buf[2]);
            printf("Reported Address is: 0x%01x\n", (buf[2] & 0x7f));
            printf("Restarting device. Should now be at i2c address 0x%01x\n", (buf[2] & 0x7f));
            toggle_gpio();
        }
    }
    return 0;
}

