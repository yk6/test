void energy (void) {
	static uint16_t ledOn = 16;
	if (getMsTick() - indicatorTime >= INDICATOR_TIME_UNIT) {
		ledOn--;
		pca9532_setLeds(ledOn,0xffff);
	}
}


// 				and acc reading when placed in function 