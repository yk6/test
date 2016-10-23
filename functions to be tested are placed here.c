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