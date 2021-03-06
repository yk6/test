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

//===============================================================================
//
//								TIMER RESET
//
//===============================================================================
	UART_Receive(LPC_UART3, &uart_transmit, strlen(uart_transmit), BLOCKING);