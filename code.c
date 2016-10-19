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
#define BLINK_TIME 333

//typedef enum {					// use enum or use a function as a whole?
//	PASSIVE =0X00,
//	DATE,
//} MODE;

int8_t xoff = 0;					//initial accelerometer calibration values
int8_t yoff = 0;
int8_t zoff = 0;


volatile uint32_t msTick = 0;		// ms clock

uint32_t led7segTime = 0;			//time of the 7 seg
uint8_t led7segCount = 0;			//current number displayed on 7seg

uint16_t ledOn = 0;					//time counters
uint32_t indicatorTime = 0;			
uint32_t rgbTime = 0;

uint8_t on_red = 0;					// flag control for rgb
uint8_t on_blue = 0;

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

	PinCfg.Funcnum=0;
	PinCfg.OpenDrain=0;
	PinCfg.Pinmode=0;
	PinCfg.Portnum=1; //P1.31
	PinCfg.Pinnum=31;

PINSEL_ConfigPin(&PinCfg);//sw4

	PinCfg.Portnum=2; // P2.10
	PinCfg.Pinnum=10;
PINSEL_ConfigPin(&PinCfg);//sw3


}


uint32_t getMsTick(void){
	return msTick;
}

void SysTick_Handler(void){     	// SysTick interrupt Handler.
	msTick++;
}

void startInit(void){
	int8_t x = 0, y = 0, z = 0;

    init_i2c();
    init_ssp();
    init_GPIO();

	acc_init();						// initiate devices
	pca9532_init();
    oled_init();
    temp_init(&getMsTick);
    led7seg_init();
    rgb_init();
	light_enable();

	calibrateAcc(x,y,z);			// start up calibration of accelerometer
	rgbInit();
	
	led7seg_setChar('*',FALSE);		
	led7segTime = getMsTick();		// set timer = current time
	indicatorTime = getMsTick();	// set energy time = current time
	rgbTime = getMsTick();			
	
}

double readTemp(int32_t t){
	t = temp_read()/10.0;			// to be improved
	return t;
}

uint32_t readLight(uint32_t l){
	l = light_read();
	return l;
}

void led7segTimer (void) {
	if ( (getMsTick() - led7segTime) >= 1000) {
		led7segCount++;
		led7segTime = getMsTick();

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
				led7seg_setChar('*',FALSE);
				break;
		}
	}
}

void energy (void) {
	static uint16_t ledOn = 0xffff;
	if (getMsTick() - indicatorTime >= INDICATOR_TIME_UNIT) {
		ledOn>>=1;
		pca9532_setLeds(ledOn,0xffff);
		indicatorTime=getMsTick();
	}
}

void calibrateAcc(int8_t x,int8_t y,int8_t z) {
	acc_read(&x,&y,&z);
	xoff = 0-x;
	yoff = 0-y;
	zoff = 0-z;
}

void rgbInit (void)
{
	PINSEL_CFG_Type PinCfg;

	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Funcnum = 0;
	PINSEL_ConfigPin(&PinCfg);		// rgb Red

	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Funcnum = 0;
	PINSEL_ConfigPin(&PinCfg);		// rgb Blue

	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 1;
	PinCfg.Funcnum = 0;
	PINSEL_ConfigPin(&PinCfg);		// rgb Green


	GPIO_SetDir(2,(1<<0),1);
	GPIO_SetDir(0,(1<<26),1);
	GPIO_SetDir(2,(1<<1),1);
	GPIO_ClearValue(2,1<<1);

}

void rgbInvert(void) {
	uint8_t red_state;
	uint8_t blue_state;

	if (on_red == 1) {
		red_state = GPIO_ReadValue(2);
		GPIO_ClearValue(2,(red_state & (1 << 0)));
		GPIO_SetValue(2,((~red_state) & (1 << 0)));
	}
	if (on_blue == 1) {
		blue_state = GPIO_ReadValue(0);
		GPIO_ClearValue(0,(blue_state & (1 << 26)));
		GPIO_SetValue(0,((~blue_state) & (1 << 26)));
	}
}

void rgbBlink(void) {
	if (getMsTick() - rgbTime >= BLINK_TIME) {
		rgbInvert();
		rgbTime = getMsTick();
	}
}

int main (void) {
	int8_t x=0 ,y=0,z=0;
	double t= 0;
	uint32_t l = 0;

	SysTick_Config(SystemCoreClock/1000);			//interrupt every ms

	startInit();
	rgb_init();
	
	on_red = 1;
	on_blue = 1;

    while (1)
    {

    	rgbBlink();


    }

}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
