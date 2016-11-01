uint32_t readTemp(void) {
    if (LPC_GPIOINT->IO0IntStatF >> 2 && (0x01)) {
    	 return getMsTick();
	}
}

then use 2 uint32_t time to store 2 value returned from this function ( each from 1 interrupt) 

uint32_t t1 = 0;
uint32_t t2 = 0; 
uint32_t t3 = 0;
	if ( t1 != 0) {
		t2 = readTemp();
		t3 = t2 - t1;
		t1 = 0;
		t2 = 0;
	} else {
		t1 = readTemp();
	}

	then find out t3 is what   then follow this formula 

	T = t3 / 10 - 273.15 ;

/*********************************************************************************************

			the top one gg cuz cant make that accurate clock 

*********************************************************************************************/


//--------------------------------------------------------------------------------------------

void TimerInit(void) {
	TIM_TIMERCFG_Type timer;
	TIM_MATCHCFG_Type match;
	
	timer.PrescaleOption = TIM_PRESCALE_USVAL;
	timer.PrescaleValue = 1;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer);
	
	match.ExtMatchOutputType = 0;
	match.IntOnMatch = ENABLE;
	match.MatchChannel = 0;
	match.MatchValue = 100;
	match.ResetOnMatch = ENABLE;
	match.StopOnMatch = DISABLE;
	TIM_ConfigMatch(LPC_TIM0, &match);
	
	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0);
}


void TIMER0_IRQHandler (void) {
	usTick++;
	LPC_TIM0 -> IR |= 0x01;
}

int main (void) {
	NVIC_SetPriority(TIMER0_IRQn,0);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);

}


/********************************************************************************



									UART



											
********************************************************************************/

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
	uart.Databits = UART_DATA_BIT_8;
	uart.Parity = UART_PARITY_NONE;
	uart.Stopbits = UART_STOPBIT_1;
	
	pinsel_uart3();
	
	UART_Init(LPC_UART3, &uart);
	UART_TxCmd(LPC_UART3, ENABLE);
}

static char *msg = NULL;

void uart(void) {
	uint8_t data = 0;
	uint32_t len = 0;
	uint8_t line[64];

	init_uart();

	msg = "Welcome to EE2024\r\n";
	UART_Send(LPC_UART3,(uint8_t *)msg, strlen(msg), BLOCKING);

	UART_Receive(LPC_UART3, &data, 1, BLOCKING);
	UART_Send(LPC_UART3, &data, 1, BLOCKING);

	len = 0;

	do {
		UART_Receive(LPC_UART3, &data, 1, BLOCKING);

		if (data != '\r') {
			len++;
			line[len-1] = data;
		}
	}while (len<64 && data != '\r');
	line[len] = 0;
	UART_SendString(LPC_UART3, &line);
	printf("--%s--\n", line);
}


//================================================================================
//							
//
//								TESTED FUNCTIONS
//
//
//================================================================================

int round(double x) {
	int n;
    x = 4.9;
    n = x + 0.5;
    return n;
}

void lightLED(void) {			//TODO
	uint16_t light;				// make it increase as a bar but not 1 led each time
	light = light_read();
	double temp = light/500.0;
	light = round(temp);
	uint32_t shift = 1<<16;
	shift >>=light;
	if (light == 0) {
		pca9532_setLeds(0,0xffff);
	} else {
		pca9532_setLeds(shift,0xffff);
	}
}
