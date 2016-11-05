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
//==================================
uint8_t uart_transmit[60] = {};

sprintf(uart_transmit, "abcdefghijkl" , );
//==================================
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

static void init_timer0(void)
{
	LPC_SC->PCONP |= (1 << 1);
	LPC_SC->PCLKSEL0 |= 0b01 << 2;
	LPC_TIM0->TCR = 1;
	LPC_TIM0->MR0 = (SystemCoreClock/(1));
	LPC_TIM0->MCR |= 1 << 0;
	LPC_TIM0->MCR |= 1 << 1;
}

void TIMER0_IRQHandler(void)
{
	if((LPC_TIM0-> IR >> 0) & 0x1)
	{
		timerCount++;
		ledTrigger = true;
		if(timerCount%2 == 0) {
			sensorTrigger = true;
		}
		LPC_TIM0-> IR |= 1;
	}
}

void UART3_IRQHandler(void) {
	uint8_t data;
	//Receive one letter
	UART_Receive(LPC_UART3, &data, 1, BLOCKING);

	// Insert letter into string buffer
	if (data!='\r')
	{
		line[len] = data;
		len++;
	}
	// Indicate string received (i.e. user pressed [ENTER] or exceeded 23 letters
	if (!sleep && ((len>=23) || (data == '\r'))) {
		UART_Receive(LPC_UART3, &data, 1, BLOCKING); // get rid of '\n' character
		line[len]=0;
		UART_message_received = true;
		// Disable UART3 INT

		NVIC_DisableIRQ(UART3_IRQn);
	}
	NVIC_ClearPendingIRQ(UART3_IRQn);
}

void printUARTmessage() {
	if (line[0] == 'S' && line[1] == 'L' && line[2] == 'E' && line[3] == 'E' && line[4] == 'P' && len == 5) {
		// Set SLEEP display
		oled_clearScreen(OLED_COLOR_BLACK);
		sprintf(value, "     SLEEP       ");
		oled_putString(1, 30, value, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

		// wait for new data
		sleep = true;
		NVIC_ClearPendingIRQ(UART3_IRQn);
		NVIC_EnableIRQ(UART3_IRQn);
		while(len==5);

		oled_clearScreen(OLED_COLOR_BLACK);
	} else {
		sprintf(value, "                %s                ", line);
		oled_putString(1, 50, value+pos, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
		++pos;
	}

	if(pos >= len+16 || sleep) {
		sprintf(value, "                     ", line);
		oled_putString(1, 50, value, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
		UART_message_received = false;
		pos = 0;
		len = 0;
		sleep = FALSE;
		// Enable UART3 INT
		NVIC_ClearPendingIRQ(UART3_IRQn);
		NVIC_EnableIRQ(UART3_IRQn);
	}
}
//===============================================================================
//
//								TIMER RESET
//
//===============================================================================

// inside the systick 

if (usTick >= 4000000000 && led7segCount == 0) { //4000s   usTick overflow at 4294s
	usTick = 0;
	msTick = 0;
	led7segTime = 0;
	indicatorTime = 0;
	rgbTime = 0;
}

//===============================================================================
reset_flag = 0;

if (usTick == 4294967295) {		//	4294967296 = 2^32 ( cuz got 0, so its not 96 but 95 )
	reset_flag = 1;
}
if (reset_flag == 1) {
	msTick = 0;
	led7segTime = 0;
	indicatorTime = 0;
	rgbTime = 0;
}


//================================================================================
//							
//
//								TESTED FUNCTIONS
//
//
//================================================================================



void lightLED(void) {			
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


	UART_Receive(LPC_UART3, &uart_transmit, strlen(uart_transmit), BLOCKING);

