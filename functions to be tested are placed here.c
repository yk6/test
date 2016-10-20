
1  change 


SysTick_Config(SystemCoreClock/1000);

to 

SysTick_Config(100000);				// interrupt  at 10µs 

/***************************************************************************




***************************************************************************/

2   change 

void SysTick_Handler(void){     	// SysTick interrupt Handler.		
	msTick++;						
}


to 

rmb to add uint32_t Tick = 0;

void SysTick_Handler(void){     	// SysTick interrupt Handler.
	Tick++;
	if (Tick%100 == 0) {
		msTick++;
	}
}

/***************************************************************************




***************************************************************************/

3    then try  led7segTimer(); 

see if it changes at rate of 1s

or use some printf to check the msTick 

or u can pass in &Tick to temp_read and 

printf the time before and after temp_read

to check whether it was read faster