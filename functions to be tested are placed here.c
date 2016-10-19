//  ways to set rgb



/**********************************************
	set up GPIO

	P2.0 is red
	P0.26 is blue 
	P2.1 is green

	then set direction  

	then 
	GPIO_ClearValue
	GPIO_SetValue
	GPIO_ReadValue



	or use rgb library     0x01 is red
						   0x02 is blue
						   0x04 is green

	current problem is can control rgb red to blink at desire freq but the blue rgb will always juz light up 

	and stop changing.

	tested with manually inputting flag to set value can work,   but dk why cant make it blink



*************************************************/