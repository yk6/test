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
#define SEGMENT_DISPLAY_TIME 1000
#define PASSIVE 1
#define DATE 2

//typedef enum {					// use enum or use a function as a whole?
//	PASSIVE =0X00,
//	DATE,
//} MODE;

int8_t xoff = 0;					//initial accelerometer calibration values
int8_t yoff = 0;
int8_t zoff = 0;
int8_t x = 0;
int8_t y = 0;
int8_t z = 0;

double t = 0.0;						//initial temp and light
uint32_t l = 0;

uint32_t mode = 0;

//	clock variables
volatile uint32_t msTick = 0;		// ms clock
volatile uint32_t usTick = 0;		// 10µs

uint32_t led7segTime = 0;			//time of the 7 seg
uint8_t led7segCount = 0;			//current number displayed on 7seg

uint32_t end_PASSIVE_button = 0;	// flag for end of passive
uint32_t end_PASSIVE = 0;			// end of passive
uint32_t update_request = 0;		// 7seg update at 5,A,F

uint32_t end_DATE = 0;				//end of date flag
uint32_t clear_date_label = 0;

uint16_t ledOn = 0;					//time counters
uint32_t indicatorTime = 0;
uint32_t rgbTime = 0;

uint8_t on_red = 0;					// flag control for rgb
uint8_t on_blue = 0;

uint8_t blue_flag = 0;				// status flag for rgb blue

//============================
//		INITIALISATION
//============================
static void init_ssp(void);
static void init_i2c(void);
static void init_GPIO(void);
void startInit(void);

//============================
//		INTERRUPTS
//============================
void EINT3_IRQHandler(void);
uint32_t getMsTick(void);
uint32_t getUsTick(void);
void SysTick_Handler(void);



//=============================
//		OLED FUNCTIONS
//=============================
void oled_value_clear (void);
void oled_update (void);
void oled_DATE_label (void);
void oled_PASSIVE_label (void);

//=============================
//		SENSORS
//=============================
void readTemp(void);
/* changed temp.c

#define NUM_HALF_PERIODS 20

return ( (2*10*t2) / (NUM_HALF_PERIODS*TEMP_SCALAR_DIV10) - 2731 );

*/
void readLight(void);
void calibrateAcc(void);

//=============================
//		LED 7 SEGMENT
//=============================
void led7segTimer (void);
void invert7seg(void);
//=============================
//		ENERGY INDICATOR
//=============================
void energy (void) ;

//============================
//		RGB FUNCTIONS
//============================
void rgbInit (void);
void rgb_off(void);
void rgbInvert(void);
void rgbBlink(void);

//=============================
//		MODE FUNCTIONS
//=============================
void PASSIVE_MODE (void);
void DATE_MODE(void);

int main (void) {
	uint8_t btn = 0;
	uint8_t start_condition = 0;			//for sw2

	SysTick_Config(SystemCoreClock/100000);			//interrupt

	startInit();


	NVIC_SetPriorityGrouping(5);			//priority setting for interrupt sw3
	NVIC_SetPriority(EINT3_IRQn, 0x18);		// 24 in DEC

	int n = 1;

    while (1)
    {
    	// invert7seg(); 
   	btn = (GPIO_ReadValue(1) >> 31) & (0x1) ;
   	printf("%d\n",btn);
		if(btn==0){
			start_condition = 1;
		}
		if(start_condition){

   		PASSIVE_MODE();
			DATE_MODE();
   	}

    }

}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}





//============================
//		INITIALISATION
//============================
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
	PinCfg.Portnum=0; //P0.17
	PinCfg.Pinnum=17;

PINSEL_ConfigPin(&PinCfg);//sw2

	PinCfg.Portnum=2; // P2.10
	PinCfg.Pinnum=10;

PINSEL_ConfigPin(&PinCfg);//sw3

	PinCfg.Portnum=1; // P1.31	removed jumper for this to work
	PinCfg.Pinnum=31;

PINSEL_ConfigPin(&PinCfg);//sw4


}

void startInit(void){
	int8_t x = 0, y = 0, z = 0;

    init_i2c();
    init_ssp();
    init_GPIO();

	acc_init();						// initiate devices
	pca9532_init();
    oled_init();
    temp_init(&getUsTick);
    led7seg_init();
    rgb_init();
    light_setRange(LIGHT_RANGE_4000);
	light_enable();

	calibrateAcc();			// start up calibration of accelerometer
	rgbInit();

	led7seg_setChar('*',FALSE);		// make 7 seg disp nothing
	oled_clearScreen(OLED_COLOR_BLACK);  // make oled screen blank

}





//============================
//		INTERRUPTS
//============================

void EINT3_IRQHandler(void){
	if((LPC_GPIOINT->IO2IntStatF >> 10)& 0x1){		//sw3 interrupt handler
		if(mode){
			update_request = 1;
		}
		LPC_GPIOINT->IO2IntClr = 1<<10;
	}
}

uint32_t getMsTick(void){
	return msTick;
}

uint32_t getUsTick(void) {
	return usTick;
}

void SysTick_Handler(void){     	// SysTick interrupt Handler.
	usTick++;
	if(usTick%100==0){
		msTick++;
	}
}





//=============================
//		OLED FUNCTIONS
//=============================


void oled_value_clear (void){
	oled_putString(32, 11, "         ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 21, "         ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 31, "         ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 41, "         ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 51, "         ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void oled_update (void){
	uint8_t str_value_temp[15] = {};
	uint8_t str_value_lux[15] = {};
	uint8_t str_value_ax[15] = {};
	uint8_t str_value_ay[15] = {};
	uint8_t str_value_az[15] = {};

	calibrateAcc();
	readLight();
	readTemp();

	sprintf(str_value_temp,"%f",t);
	sprintf(str_value_lux,"%d",l);
	sprintf(str_value_ax,"%d",xoff);
	sprintf(str_value_ay,"%d",yoff);
	sprintf(str_value_az,"%d",zoff);

	str_value_temp[4] = '\0';

	if(l<1000){
		str_value_lux[3]=' ';				//make sure it is displayed right
		if(l<100){
			str_value_lux[2]=' ';
			if(l<10){
				str_value_lux[1]=' ';
			}
		}
	}

	if(xoff > 99){
		str_value_ax[3] = ' ';									//100-128
	}
	if((xoff > -10 && xoff < 0)||(xoff > 9 && xoff < 100)){		//-9to-1, 10-99
		str_value_ax[2] = ' ';
		str_value_ax[3] = ' ';
	}
	if((xoff > -1)&&(xoff < 10)){								//0-9
		str_value_ax[1] = ' ';
		str_value_ax[2] = ' ';
		str_value_ax[3] = ' ';
	}
	if(yoff > 99){
		str_value_ay[3] = ' ';									//100-128
	}
	if((yoff > -10 && yoff < 0)||(yoff > 9 && yoff < 100)){		//-9to-1, 10-99
		str_value_ay[2] = ' ';
		str_value_ay[3] = ' ';
	}
	if((yoff > -1)&&(yoff < 10)){								//0-9
		str_value_ay[1] = ' ';
		str_value_ay[2] = ' ';
		str_value_ay[3] = ' ';
	}
	if(zoff > 99){
		str_value_az[3] = ' ';									//100-128
	}
	if((zoff > -10 && zoff < 0)||(zoff > 9 && zoff < 100)){		//-9to-1, 10-99
		str_value_az[2] = ' ';
		str_value_az[3] = ' ';
	}
	if((zoff > -1)&&(zoff < 10)){								//0-9
		str_value_az[1] = ' ';
		str_value_az[2] = ' ';
		str_value_az[3] = ' ';
	}

	oled_putString(32, 11, (uint8_t*)str_value_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 21, (uint8_t*)str_value_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 31, (uint8_t*)str_value_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 41, (uint8_t*)str_value_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 51, (uint8_t*)str_value_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

}

void oled_DATE_label (void){
	uint8_t str_passive[15] = {"MODE: DATE"};
	uint8_t str_label_temp[15] = {"TEMP:"};
	uint8_t str_label_lux[15] = {"LUX:"};
	uint8_t str_label_ax[15] = {"AX:"};
	uint8_t str_label_ay[15] = {"AY:"};
	uint8_t str_label_az[15] = {"AZ:"};
	uint8_t str_value_temp[15] = {"DATE MODE"};
	uint8_t str_value_lux[15] = {"DATE MODE"};
	uint8_t str_value_ax[15] = {"DATE MODE"};
	uint8_t str_value_ay[15] = {"DATE MODE"};
	uint8_t str_value_az[15] = {"DATE MODE"};

	oled_putString(2, 1, (uint8_t*)str_passive, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 11, (uint8_t*)str_label_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 21, (uint8_t*)str_label_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 31, (uint8_t*)str_label_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 41, (uint8_t*)str_label_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 51, (uint8_t*)str_label_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 11, (uint8_t*)str_value_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 21, (uint8_t*)str_value_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 31, (uint8_t*)str_value_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 41, (uint8_t*)str_value_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(32, 51, (uint8_t*)str_value_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

	clear_date_label = 1;
}

void oled_PASSIVE_label (void){
	uint8_t str_passive[15] = {"MODE: PASSIVE"};
	uint8_t str_label_temp[15] = {"TEMP:"};
	uint8_t str_label_lux[15] = {"LUX:"};
	uint8_t str_label_ax[15] = {"AX:"};
	uint8_t str_label_ay[15] = {"AY:"};
	uint8_t str_label_az[15] = {"AZ:"};

	oled_putString(2, 1, (uint8_t*)str_passive, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 11, (uint8_t*)str_label_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 21, (uint8_t*)str_label_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 31, (uint8_t*)str_label_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 41, (uint8_t*)str_label_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(2, 51, (uint8_t*)str_label_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}





//=============================
//		SENSORS
//=============================
void readTemp(void){
	t = temp_read()/10.0;			// to be improved
}

void readLight(void){
	l = light_read();
}

void calibrateAcc(void) {
	acc_read(&x,&y,&z);
	xoff = 0-x;
	yoff = 0-y;
	zoff = 0-z;
}





//=============================
//		LED 7 SEGMENT
//=============================
void led7segTimer (void) {
	uint32_t time_diff = 0;

	time_diff = getMsTick() - led7segTime;
	if ( (time_diff) >= SEGMENT_DISPLAY_TIME) {
		led7segCount++;
		time_diff = time_diff - SEGMENT_DISPLAY_TIME;	//excess time
		led7segTime = getMsTick() - time_diff;			//so that overall 0-F is 16 seconds exact

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
				update_request = 1;
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
				update_request = 1;
				break;
			case 11:
				led7seg_setChar('8',FALSE);
				break;
			case 12:
				led7seg_setChar('C',FALSE);
				break;
			case 13:
				led7seg_setChar('0',FALSE);
				break;
			case 14:
				led7seg_setChar('E',FALSE);
				break;
			case 15:
				if(end_PASSIVE_button){				// condition fulfilled if button pressed before F shows up
					end_PASSIVE = 1;
					end_PASSIVE_button = 0;
				}
				led7seg_setChar('F',FALSE);
				update_request = 1;
				break;
			default:
				led7seg_setChar('*',FALSE);
				break;
		}
	}
}

void invert7seg(void) {
	if (getMsTick() - led7segTime>= SEGMENT_DISPLAY_TIME) {
		led7segTime=getMsTick();
		if(led7segCount==16) {
			led7segCount=0;
		}
		switch(led7segCount) {
			case 0:
				led7seg_setChar(0x24,TRUE);
				break;
			case 1:
				led7seg_setChar(0x7D,TRUE);
				break;
			case 2:
				led7seg_setChar(0xE0,TRUE);
				break;
			case 3:
				led7seg_setChar(0x70,TRUE);
				break;
			case 4:
				led7seg_setChar(0x39,TRUE);
				break;
			case 5:
				led7seg_setChar(0x32,TRUE);
				break;
			case 6:
				led7seg_setChar(0x22,TRUE);
				break;
			case 7:
				led7seg_setChar(0x7C,TRUE);
				break;
			case 8:
				led7seg_setChar(0x20,TRUE);
				break;
			case 9:
				led7seg_setChar(0x30,TRUE);
				break;
			case 10:
				led7seg_setChar(0x28,TRUE);
				break;
			case 11:
				led7seg_setChar(0x20,TRUE);
				break;
			case 12:
				led7seg_setChar(0xA6,TRUE);
				break;
			case 13:
				led7seg_setChar(0x24,TRUE);
				break;
			case 14:
				led7seg_setChar(0xA2,TRUE);
				break;
			case 15:
				led7seg_setChar(0xAA,TRUE);
				break;
			default:;
		}
		led7segCount++;
	}
}





//=============================
//		ENERGY INDICATOR
//=============================
void energy (void) {
	uint32_t time_diff = 0;
	static uint16_t ledOn = 0xffff;

	time_diff = getMsTick() - indicatorTime;
	if (time_diff >= INDICATOR_TIME_UNIT) {
		ledOn>>=1;
		pca9532_setLeds(ledOn,0xffff);
		time_diff = time_diff - INDICATOR_TIME_UNIT;			//excess time
		indicatorTime = getMsTick() - time_diff;
	}
	if(ledOn == 0){
		end_DATE = 1;
		ledOn = 0xffff;
	}
}





//=============================
//		RGB FUNCTIONS
//=============================
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
//	GPIO_ClearValue(2,1<<1);		// removed rgb green jumper

}

void rgb_off(void){
	on_blue = 0;
	on_red = 0;
	GPIO_ClearValue(0,1<<26);
	GPIO_ClearValue(2,1<<0);
}

void rgbInvert(void) {
	uint8_t red_state;
//	uint8_t blue_state;

	if (on_red == 1) {
		red_state = GPIO_ReadValue(2);
		GPIO_ClearValue(2,(red_state & (1 << 0)));
		GPIO_SetValue(2,((~red_state) & (1 << 0)));
	}
//	if (on_blue == 1) {									// somehow this method dont work for blue
//		blue_state = GPIO_ReadValue(0);
//		GPIO_ClearValue(0,(blue_state & (1 << 26)));
//		GPIO_SetValue(0,((~blue_state) & (1 << 26)));
//	}
	if (on_blue == 1) {
		if (blue_flag == 0) {
			GPIO_SetValue(0,1<<26);
			blue_flag=1;
		} else if (blue_flag == 1) {
			GPIO_ClearValue(0,1<<26);
			blue_flag=0;
		}
	}

}

void rgbBlink(void) {
	if (getMsTick() - rgbTime >= BLINK_TIME) {
		rgbInvert();
		rgbTime = getMsTick();
	}
}





//=============================
//		MODE FUNCTIONS
//=============================
void PASSIVE_MODE (void){
	uint8_t btn = 0;

	mode = 0;
	if(clear_date_label){
		oled_value_clear();
		clear_date_label = 0;
	}
	oled_PASSIVE_label();		//print labels and first update of values
	oled_update();
	rgbTime = getMsTick();
	led7segTime = getMsTick();		// set timer = current time
	while(1){
		rgbBlink();				//call blink outside if condition to ensure constant blinking
		led7segTimer();			//turn on the 7segtimer
		if(update_request){		//update_request happens at 5,A,F
			oled_update();
			update_request = 0;
			if((l<50)&&(on_red==0)){		//conditions for which color to blink
				on_red = 1;
				on_blue = 1;
				rgbBlink();				//call blink to synchronize
			}
			if((l>=50)&&(l<=1000)&&(on_blue==0)){		//conditions for which color to blink
				on_blue = 1;
			}
		}


//------------------button pressed conditions from here below---------------
		btn = (GPIO_ReadValue(1) >> 31) & (0x1) ;
		if(btn==0){
			end_PASSIVE_button = 1;
		}
		if(end_PASSIVE){
			end_PASSIVE = 0;
			break;
		}
	}

}

void DATE_MODE(void){
	mode = 1;
	rgb_off();
	led7seg_setChar('*',FALSE);		// make 7 seg disp nothing
	oled_DATE_label();
	indicatorTime = getMsTick();	// set energy time = current time
	while(1){
		energy();
		LPC_GPIOINT->IO2IntEnF |= 1<<10;
		NVIC_EnableIRQ(EINT3_IRQn);					//enable interrupt
		if(update_request){
			if(clear_date_label){
				oled_value_clear();
				clear_date_label = 0;
			}
			oled_update();
			update_request = 0;
		}
		if(end_DATE){
			end_DATE = 0;
			break;
		}
	}
}