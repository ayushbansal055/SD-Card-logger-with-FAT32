#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "ff.h"
#include "diskio.h"

DWORD get_fattime (void)
{
    return ((DWORD)(2025 - 1980) << 25) // Year = 2025 (45)
          | ((DWORD)1 << 21)           // Month = 1 (January)
          | ((DWORD)1 << 16)           // Day = 1
          | ((DWORD)0 << 11)           // Hour = 0
          | ((DWORD)0 << 5)            // Minute = 0
          | ((DWORD)0 >> 1);           // Second = 0
}

void LED_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
                          GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

void LED(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t val = 0;
    if(r) val |= GPIO_PIN_1;
    if(b) val |= GPIO_PIN_2;
    if(g) val |= GPIO_PIN_3;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, val);
}

void Delay_ms(uint32_t ms)
{
    SysCtlDelay((SysCtlClockGet()/3000)*ms);
}

FATFS fs;
FIL file;
UINT bw, br;
uint8_t buf[32];

int main(void)
{
    char line[32];
    UINT bw;
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    LED_Init();
    LED(1,1,1);
        Delay_ms(500);


    DSTATUS stat = disk_initialize(0);

    if (stat & STA_NOINIT) {
           // HARDWARE FAILURE
        while(1) {
            LED(1,0,0); // Blink RED fast
            Delay_ms(100);
            LED(0,0,0);
            Delay_ms(100);
        }
    }

        // Hardware is Good!
    
        LED(0,0,1); // BLUE: Hardware OK
        Delay_ms(500);

    //Stage 1: Mount SD 
    if (f_mount(0, &fs) != FR_OK)
    {
        while(1) LED(1,0,0); // ERROR red
    }

    LED(0,1,0);   // GREEN: mounted
    Delay_ms(500);


    // Stage 2: Open the exixting file
    
        f_open(&file, "tests.txt", FA_WRITE | FA_OPEN_ALWAYS);  // Open
        f_lseek(&file, f_size(&file));                         // Move to end of file
        f_write(&file, "APPENDED to LINE\n", 18, &bw);             // write new data


    LED(1,1,0);   // YELLOW: write done

    int i;
    for ( i = 11; i <= 20; i++)
    {
        f_lseek(&file, f_size(&file));       // move pointer to end

        // convert i into text
        sprintf(line, "%d\n", i);

        f_write(&file, line, strlen(line), &bw);
    }
    f_close(&file);

    LED(1,0,1);   // PURPLE: File close



    while(1)
    {
        LED(1,0,1);
        Delay_ms(300);
        LED(0,0,0);
        Delay_ms(300);
    }
}
