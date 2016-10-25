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
	// make faster SPI for oled

	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.ClockRate = 8000000;
	

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


/*-------------------------------------------------------------------------------


********************************************************************************/

SysTick_Config(SystemCoreClock/1000000);

pass into init_temp();

then change temp_read();    to µs as unit   maybe can try sample at lesser NUM_HALFCYCLE


then try change SystemCoreClock making it variable 

