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


/*-------------------------------------------------------------------------------


********************************************************************************/

void invert7seg(void) {
	if (getMsTick() - led7segTime>= SEGMENT_DISPLAY_TIME) {
		if(led7segCount==16){
			led7segCount=0;
		}
		led7segTime=getMsTick();
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