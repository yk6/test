#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"


#include "acc.h"
#include "led7seg.h"
#include "light.h"
#include "oled.h"
#include "pca9532.h"
#include "rgb.h"
#include "temp.h"

#define WARNING_LOWER 50
#define WARNING_UPPER 1000
#define INDICATOR_TIME_UNIT 208

//typedef enum {				// use enum or use a function as a whole?
//	PASSIVE =0X00,
//	DATE,
//} MODE;

int32_t xoff = 0;
int32_t yoff = 0;
int32_t zoff = 0;

int8_t x = 0;
int8_t y = 0;
int8_t z = 0;

volatile uint32_t msTick = 0;

uint32_t led7segTime = 0;
uint8_t led7segCount = 0;

uint32_t l = 0;

int32_t t = 0;

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void)
{
	// Initialize button
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0;
	PinCfg.Pinnum = 4;
	PinCfg.Portnum = 0;   //sw3 bl en
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, 1<<4, 0);  //set sw3 as input

	PinCfg.Pinnum = 9;
	PinCfg.Portnum = 1;
	PINSEL_ConfigPin(&PinCfg);     //red rgb
	GPIO_SetDir(1, 1<<9, 1);  //set as output

	PinCfg.Pinnum = 2;
	PinCfg.Portnum = 1;
	PINSEL_ConfigPin(&PinCfg);     //green rgb
	GPIO_SetDir(1, 1<<2, 1);  //set as output

	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 1;
	PINSEL_ConfigPin(&PinCfg);    //blue rgb
	GPIO_SetDir(1, 1<<10, 1);  //set as output


}


uint32_t getMsTick(void){
	return msTick;
}

void SysTick_Handler(void){                               /* SysTick interrupt Handler.*/
  msTick++;
}

void EINT3_IRQHandler (void){
	if((LPC_GPIOINT->IO0IntStatF>>4)&0x1){			// interrupt handler
		LPC_GPIOINT->IO0IntClr = 1<<4;
	}
}

void calibrateAcc(void) {
    acc_read(&x,&y,&z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;
}

void startInitialise (void){

	SysTick_Config(SystemCoreClock / 1000);			//interrupt every ms

    init_i2c();
    init_ssp();
    init_GPIO();

	acc_init();					// initiate devices
	pca9532_init();
    oled_init();
    temp_init(&getMsTick);
    led7seg_init();
    rgb_init();
	light_enable();

	calibrateAcc();				// start up calibration of accelerometer


}

void readTemp(uint32_t *t){
	t = temp_read();
}

void readLight(uint32_t *l){
	l = light_read();
}

void readAcc(void) {
	acc_read(&x,&y,&z);
	x += xoff;
	y += yoff;
	z += zoff;
}

void led7segTimer (void) {
	if ( (getMsTick() - led7segTime) > 1000) {
		led7segCount++;
		if (led7segCount == 16){
			led7segCount = 0;
		}
		switch (led7segCount) {
			case 0:
				led7seg_setChar('0',FALSE);
				break;
			case 1:
				led7seg_setChar('1',FALSE);
				break;
			case 2:
				led7seg_setChar('2',FALSE);
				break;
			case 3:
				led7seg_setChar('3',FALSE);
				break;
			case 4:
				led7seg_setChar('4',FALSE);
				break;
			case 5:
				led7seg_setChar('5',FALSE);
				break;
			case 6:
				led7seg_setChar('6',FALSE);
				break;
			case 7:
				led7seg_setChar('7',FALSE);
				break;
			case 8:
				led7seg_setChar('8',FALSE);
				break;
			case 9:
				led7seg_setChar('9',FALSE);
				break;
			case 10:
				led7seg_setChar('A',FALSE);
				break;
			case 11:
				led7seg_setChar('B',FALSE);
				break;
			case 12:
				led7seg_setChar('C',FALSE);
				break;
			case 13:
				led7seg_setChar('D',FALSE);
				break;
			case 14:
				led7seg_setChar('E',FALSE);
				break;
			case 15:
				led7seg_setChar('F',FALSE);
				break;
			default:
				led7seg_setChar(' ',FALSE);
				break;
		}
	}
}

void PASSIVE(void){

	uint8_t ch[40] = {'PASSIVE'};

	oled_putString(30,20,(uint8_t*)ch, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

	readAcc();
	readTemp();
	readLight();
}

void DATE (void){
	readAcc();
}


int main (void) {
	uint32_t PASSIVE_MODE = 0;
	uint32_t DATE_MODE = 0;
	uint32_t SAFE_MODE = 0;

	uint32_t MODE_TOGGLE = 0;

	uint8_t mode = 0;






    while (1)
    {
//    	LPC_GPIOINT->IO0IntEnF |= 1<<4;		//enable interrupt
//    	NVIC_EnableIRQ(EINT3_IRQn);
    	MODE_TOGGLE = (GPIO_ReadValue(0)>>4)&0x1;





    	Timer0_Wait(1);
    }

}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
