#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

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


volatile int8_t xoff = 0;						//initial accelerometer calibration values
volatile int8_t yoff = 0;
volatile int8_t zoff = 0;
int8_t x = 0;
int8_t y = 0;
int8_t z = 0;

volatile double t = 0.0;						//initial temp and light
volatile uint32_t l = 0;

volatile uint32_t mode = 0;

												//	clock variables
volatile uint32_t msTick = 0;					// 1ms clock
volatile uint32_t usTick = 0;					// 1µs

volatile uint32_t led7segTime = 0;				//time of the 7 seg
volatile uint8_t led7segCount = 0;				//current number displayed on 7seg
volatile uint8_t invert_7seg = 0;

volatile uint32_t end_PASSIVE_button = 0;		// flag for end of passive
volatile uint32_t end_PASSIVE = 0;				// end of passive
volatile uint8_t change_mode = 0;				// the instance when passive switch to date
volatile uint32_t update_request = 0;			// 7seg update at 5,A,F

volatile uint32_t end_DATE = 0;					//end of date flag
volatile uint32_t clear_date_label = 0;

volatile uint32_t indicatorTime = 0;			//time counters
volatile uint32_t rgbTime = 0;

volatile uint8_t on_red = 0;					// flag control for rgb
volatile uint8_t on_blue = 0;

volatile uint8_t blue_flag = 0;					//	status flag for rgb blue
volatile uint8_t red_flag = 0;					//	status flag for rgb red

volatile uint8_t uart_transmit[100];			// UART
volatile uint32_t transmit_count = 0;
volatile uint8_t uart_receive[100];
volatile uint32_t count = 0;
volatile uint8_t got_msg = 0;

uint32_t passive_batt_cycle = 0;				//	battery
uint32_t date_batt_cycle = 0;
double total_batt = 100.0;
uint8_t sent10 = 0;								// flag to prevent spamming of uart
uint8_t sent5 = 0;
uint8_t batt_buf[50];

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
void oled_update (void);
void oled_DATE_label (void);
void oled_DATE_label_value (void);
void oled_PASSIVE_label (void);
void oled_labels(void);

//=============================
//		SENSORS
//=============================
void readTemp(void);
/* changed temp.c

#define NUM_HALF_PERIODS 30

return ( (2*10*t2) / (NUM_HALF_PERIODS*TEMP_SCALAR_DIV10) - 27315 );

*/
void readLight(void);
void calibrateAcc(int8_t xx, int8_t yy, int8_t zz);

//=============================
//		LED 7 SEGMENT
//=============================
void led7segTimer (void);
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

//=============================
//		UART FUNCTIONS
//=============================
void pinsel_uart3(void);
void init_uart(void);
void send_UartData(void);
void computeState(void);
void clearUartBuf(void);
void UART3_IRQHandler(void);

//=============================
//		BATT FUNCTIONS
//=============================
void UpdateBattery_level(void);
void low_batt(void);
void checkUartMsg(void);
void printBattStat(void);


int main (void) {
	uint8_t btn = 0;
	uint8_t falling_edge = 0;
	uint8_t start_condition = 0;						//for sw2

	SysTick_Config(SystemCoreClock/1000000);			//interrupt at 1µｓ

	startInit();

	NVIC_EnableIRQ(EINT3_IRQn);							//enable interrupt
	NVIC_SetPriorityGrouping(5);						//priority setting for interrupt sw3
	NVIC_SetPriority(EINT3_IRQn, 0x18);					// 24 in DEC
	LPC_GPIOINT->IO2IntEnF |= 1<<10;
	LPC_GPIOINT->IO0IntEnF |= 1<<17;

	NVIC_EnableIRQ(UART3_IRQn);							//enable uart interrupt
	UART_IntConfig(LPC_UART3, UART_INTCFG_RBR, ENABLE);


//=================================================================================


    while (1)
    {
//========================================================

    	if (total_batt != total_batt) {
    		printf("batt lv: %.2lf\n", total_batt);
    	}


//========================================================



//    	btn = (GPIO_ReadValue(1) >> 31) & (0x1) ;
//		if(btn==0){
//			falling_edge = 1;
//		}
//		if((btn==1) && (falling_edge==1)){
//			start_condition = 1;
//		}
//		if(start_condition){
//
//			PASSIVE_MODE();
//			DATE_MODE();
//		}




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
	SSP_ConfigStruct.ClockRate = 20000000;	//20Mhz

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
	int8_t xx = 0, yy = 0, zz = 0;

    init_i2c();
    init_ssp();
    init_GPIO();

	acc_init();								// initiate devices
	pca9532_init();
    oled_init();
    temp_init(&getUsTick);
    led7seg_init();
    rgb_init();
    light_enable();
    light_setRange(LIGHT_RANGE_4000);
    init_uart();

	calibrateAcc(xx,yy,zz);					// start up calibration of accelerometer
	rgbInit();

	led7seg_setChar('*',FALSE);				// make 7 seg disp nothing
	oled_clearScreen(OLED_COLOR_BLACK);  	// make oled screen blank

}





//============================
//		INTERRUPTS
//============================

void EINT3_IRQHandler(void){
	if((LPC_GPIOINT->IO2IntStatF >> 10)& 0x1){		//sw3 interrupt handler
		if(mode == 1){
			update_request = 1;
		}
		LPC_GPIOINT->IO2IntClr = 1<<10;
	}
	if((LPC_GPIOINT->IO0IntStatF >> 17)& 0x1){		//joystick interrupt handler
		if(mode == 0){
			if(invert_7seg == 0){
				invert_7seg = 1;
			}else{
				invert_7seg = 0;
			}
		}
		LPC_GPIOINT->IO0IntClr = 1<<17;
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
	if(usTick%1000==0){
		msTick++;
	}
}





//=============================
//		OLED FUNCTIONS
//=============================


void oled_update (void){
	uint8_t str_value_temp[15] = {};
	uint8_t str_value_lux[15] = {};
	uint8_t str_value_ax[15] = {};
	uint8_t str_value_ay[15] = {};
	uint8_t str_value_az[15] = {};

	acc_read(&x,&y,&z);
	x = x+xoff;
	y = y+yoff;
	z = z+zoff;
	readLight();
	readTemp();
	//50ms

	sprintf(str_value_temp,"%.2f",t);
	oled_putString(16, 16, (uint8_t*)str_value_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	sprintf(str_value_lux,"%4d",l);
	oled_putString(16, 24, (uint8_t*)str_value_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	sprintf(str_value_ax,"%3d",x);
	oled_putString(16, 32, (uint8_t*)str_value_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	sprintf(str_value_ay,"%3d",y);
	oled_putString(16, 40, (uint8_t*)str_value_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	sprintf(str_value_az,"%3d",z);
	oled_putString(16, 48, (uint8_t*)str_value_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void oled_DATE_label_value (void){

	uint8_t str_value_temp[15] = {"DATE MODE"};
	oled_putString(16, 16, (uint8_t*)str_value_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_value_lux[15] = {"DATE MODE"};
	oled_putString(16, 24, (uint8_t*)str_value_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_value_ax[15] = {"DATE MODE"};
	oled_putString(16, 32, (uint8_t*)str_value_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_value_ay[15] = {"DATE MODE"};
	oled_putString(16, 40, (uint8_t*)str_value_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_value_az[15] = {"DATE MODE"};
	oled_putString(16, 48, (uint8_t*)str_value_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

	clear_date_label = 1;

}

void oled_DATE_label (void){
	uint8_t str_date[15] = {"DATE"};

	oled_putString(0, 0, (uint8_t*)str_date, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void oled_PASSIVE_label (void){
	uint8_t str_passive[15] = {"PASSIVE"};

	oled_putString(0, 0, (uint8_t*)str_passive, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void oled_labels(void) {
	uint8_t str_label_temp[15] = {"T :"};
	oled_putString(0, 16, (uint8_t*)str_label_temp, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_label_lux[15] = {"L :"};
	oled_putString(0, 24, (uint8_t*)str_label_lux, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_label_ax[15] = {"AX:"};
	oled_putString(0, 32, (uint8_t*)str_label_ax, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_label_ay[15] = {"AY:"};
	oled_putString(0, 40, (uint8_t*)str_label_ay, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	uint8_t str_label_az[15] = {"AZ:"};
	oled_putString(0, 48, (uint8_t*)str_label_az, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}





//=============================
//		SENSORS
//=============================
void readTemp(void){
	t = temp_read()/100.0;
}

void readLight(void){
	l = light_read();
}

void calibrateAcc(int8_t xx, int8_t yy, int8_t zz) {
	acc_read(&xx,&yy,&zz);
	xoff = 0-xx;
	yoff = 0-yy;
	zoff = 64-zz;
}




//=============================
//		LED 7 SEGMENT
//=============================
void led7segTimer (void) {
	uint32_t time_diff = 0;

	if(indicatorTime > getMsTick()){									//mstik overflow condition
		time_diff = (0xFFFFFFFF - led7segTime + 1) + getMsTick();
	}else{
		time_diff = getMsTick() - led7segTime;
	}
	if ( (time_diff) >= SEGMENT_DISPLAY_TIME) {
		led7segCount++;
		time_diff = time_diff - SEGMENT_DISPLAY_TIME;	//excess time
		led7segTime = getMsTick() - time_diff;			//so that overall 0-F is 16 seconds exact

		if (led7segCount == 16){
			led7segCount = 0;
		}
		if(invert_7seg){
			switch (led7segCount) {
				case 0:
					if (end_PASSIVE == 1) {
						change_mode = 1;
						end_PASSIVE = 0;
					}
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
					update_request = 1;
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
					update_request = 1;
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
					if(end_PASSIVE_button){				// condition fulfilled if button pressed before F shows up
						end_PASSIVE = 1;
						end_PASSIVE_button = 0;
					}
					led7seg_setChar(0xAA,TRUE);
					if (on_blue == 1) {
						clearUartBuf();
						sprintf(uart_transmit, "Algae was Detected.\r\n");
						send_UartData();
					}
					if (on_red == 1) {
						clearUartBuf();
						sprintf(uart_transmit, "Solid Wastes was Detected.\r\n");
						send_UartData();
					}
					computeState();
					send_UartData();
					update_request = 1;
					break;
				default:
					led7seg_setChar('*',FALSE);
					break;
			}
		}else{
			switch (led7segCount) {
				case 0:
					if (end_PASSIVE == 1) {
						change_mode = 1;
						end_PASSIVE = 0;
					}
					led7seg_setChar(0x24,TRUE);
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
					if (on_blue == 1) {
						clearUartBuf();
						sprintf(uart_transmit, "Algae was Detected.\r\n");
						send_UartData();
					}
					if (on_red == 1) {
						clearUartBuf();
						sprintf(uart_transmit, "Solid Wastes was Detected.\r\n");
						send_UartData();
					}
					computeState();
					send_UartData();
					update_request = 1;
					break;
				default:
					led7seg_setChar('*',FALSE);
					break;
			}

		}
	}
}






//=============================
//		ENERGY INDICATOR
//=============================
void energy (void) {
	uint32_t time_diff = 0;
	static uint16_t ledOn = 0xffff;

	if(indicatorTime > getMsTick()){									//mstik overflow condition
		time_diff = (0xFFFFFFFF - indicatorTime + 1) + getMsTick();
	}else{
		time_diff = getMsTick() - indicatorTime;
	}
	if (time_diff >= INDICATOR_TIME_UNIT) {
		ledOn>>=1;
		time_diff = time_diff - INDICATOR_TIME_UNIT;			//excess time
		indicatorTime = getMsTick() - time_diff;
	}
	pca9532_setLeds(ledOn,0xffff);
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

	if (on_red == 1) {
		if (red_flag == 0) {
			GPIO_SetValue(2,1 << 0);
			red_flag = 1;
		} else if (red_flag == 1) {
			GPIO_ClearValue(2,1 << 0);
			red_flag = 0;
		}
	}
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
	uint8_t date_impending[1] = {};

	clearUartBuf();
	sprintf(uart_transmit, "Entering PASSIVE Mode.\r\n");
	send_UartData();
	clearUartBuf();
	mode = 0;
	update_request = 0;
	end_PASSIVE_button = 0;
	end_PASSIVE = 0;
	invert_7seg = 0;
	on_red = 0;
	on_blue = 0;									//initialise onblink values

	oled_clearScreen(OLED_COLOR_BLACK);
	led7segTime = getMsTick();						// set timer = current time
	led7seg_setChar(0x24,TRUE); 					//set the 7seg to 0
	oled_PASSIVE_label();							//print labels and first update of values
	oled_labels();
	oled_update();
	rgbTime = getMsTick();
	while(1){
		rgbBlink();									//call blink outside if condition to ensure constant blinking
		led7segTimer();								//start the 7segtimer
		if(update_request){							//update_request happens at 5,A,F
			oled_update();
			update_request = 0;
			if((l<50)&&(on_red==0)){				//conditions for which color to blink
				on_red = 1;
				blue_flag = 0;						//view blue as off to on when blinkInvert is called
				rgbBlink();							//call blink to synchronize
			}
			if(((l>=50)&&(l<=1000))&&(on_blue==0)){	//conditions for which color to blink
				on_blue = 1;
				red_flag = 0;
				rgbBlink();
			}
		}


//------------------button pressed conditions from here below---------------
		btn = (GPIO_ReadValue(1) >> 31) & (0x1) ;
		if(btn==0){
			end_PASSIVE_button = 1;
			sprintf(date_impending,"D");			//show a D when button pressed so will know it will change to date soon
			oled_putString(79, 1, (uint8_t*)date_impending, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
		}
		if(change_mode){
			change_mode = 0;
			sprintf(date_impending," ");			//clear the D
			oled_putString(79, 1, (uint8_t*)date_impending, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			passive_batt_cycle++;
			break;
		}
	}

}


void DATE_MODE(void){
	update_request = 0;
	mode = 1;

	clearUartBuf();
	sprintf(uart_transmit, "Leaving PASSIVE Mode. Entering DATE Mode.\r\n");
	send_UartData();
	rgb_off();
	led7seg_setChar('*',FALSE);		// make 7 seg disp nothing
	oled_DATE_label();
	oled_DATE_label_value();
	indicatorTime = getMsTick();	// set energy time = current time
	while(1){
		energy();
		if(update_request){
			if(clear_date_label){
				oled_clearScreen(OLED_COLOR_BLACK);
				oled_DATE_label();
				oled_labels();
				clear_date_label = 0;
			}
			oled_update();
			computeState();
			send_UartData();
			update_request = 0;
		}
		if(end_DATE){
			end_DATE = 0;
			date_batt_cycle++;
			break;
		}
	}
}

//==============================
//		MATH FUNCTION
//==============================

int round(double x) {
	int n;
    n = x + 0.5;
    return n;
}

//=============================
//		UART FUNCTIONS
//=============================

void pinsel_uart3(void) {
	PINSEL_CFG_Type pin;
	pin.Funcnum = 2;
	pin.Pinnum = 0;
	pin.Portnum = 0;
	PINSEL_ConfigPin(&pin);
	pin.Pinnum = 1;
	PINSEL_ConfigPin(&pin);
}

void init_uart(void) {
	UART_CFG_Type uart;
	uart.Baud_rate = 115200;
	uart.Databits = UART_DATABIT_8;
	uart.Parity = UART_PARITY_NONE;
	uart.Stopbits = UART_STOPBIT_1;

	pinsel_uart3();

	UART_Init(LPC_UART3, &uart);
	UART_TxCmd(LPC_UART3, ENABLE);
}

void send_UartData(void) {
	UART_Send(LPC_UART3, (uint8_t *)uart_transmit, strlen(uart_transmit), BLOCKING);
}

void computeState(void) {
	sprintf(uart_transmit, "%03d_-_T%2.2lf_L%4d_AX%d_AY%d_AZ%d\r\n", transmit_count, t, l, x, y, z);
	transmit_count++;
}

void clearUartBuf(void) {
	int i, len;
	len = strlen(uart_transmit);
	for (i=0; i<len; i++) {
		uart_transmit[i] = 0;
	}
}



void UART3_IRQHandler(void) {
	uint8_t data;

	UART_Receive(LPC_UART3, &data, 1, BLOCKING);

	if (data!='\r')
	{
		uart_receive[count] = data;
		count++;
	}

	if ((data == '\r')) {
		UART_Receive(LPC_UART3, &data, 1, BLOCKING);
		uart_receive[count]=0;
		got_msg = 1;
	}
	NVIC_ClearPendingIRQ(UART3_IRQn);
}

//=============================
//		BATT FUNCTIONS
//=============================
// only updates battery in passive mode

void UpdateBattery_level (void) {
	// 1 cycle of passive uses 0.05% of total, 1 cycle of date uses 1%
	total_batt -= (passive_batt_cycle * 0.05 + date_batt_cycle * 1.0);
}


void low_batt(void) {
	// if batt lower than 10%  send warning on the next instance of 7seg change
	// if batt lower than 5%   auto "shutdown"  sleep mode ------>   the mode before entering passive
	if (total_batt < 10.0 && sent10 == 0) {
		clearUartBuf();
		sprintf(uart_transmit, "LOW BATTERY! Going into sleep mode soon.\nThe battter level is %.2lf\r\n", total_batt);
		sent10 = 1;
		// TODO
		// make this send/show only when 7seg reach 0

	}
	if (total_batt < 5.0 && sent5 == 0) {
		//	send to uart once and make sleep
		clearUartBuf();
		sprintf(uart_transmit, "GOING INTO SLEEP MODE!\nThe battter level is %.2lf\r\n", total_batt);
		send_UartData();
		sent5 = 1;
		// TODO
		// go to sleep mode
	}
}

void checkUartMsg(void) {
	if (uart_receive[0] == 'B' && uart_receive[1] == 'A' && uart_receive[2] == 'T' && uart_receive[3] == 'T' && count == 4) {
		// send BATT level thru UART
		// if uart received "BATT"
		clearUartBuf();
		sprintf(uart_transmit, "The battter level is %.2lf\r\n", total_batt);
		send_UartData();
	}
	if (uart_receive[0] == 'S' && uart_receive[1] == 'L' && uart_receive[2] == 'E' && uart_receive[3] == 'E' &&  uart_receive[4] == 'P') {
		// if uart received "SLEEP"
		// go to sleep mode
	}
}

void printBattStat(void) {
	// when some switch is pushed to sth, show batt state on oled   but not in date mode preferablely
	oled_clearScreen(OLED_COLOR_BLACK);
	sprintf(batt_buf, "Battery Level:%.2lf", total_batt);
	oled_putString(0, 31, (uint8_t*)batt_buf, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}
