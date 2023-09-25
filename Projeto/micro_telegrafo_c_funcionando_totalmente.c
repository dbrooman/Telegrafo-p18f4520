//Programa pra piscar o led

#pragma config OSC = HS				//Clock set to High-Speed mode (20 MHz)
#pragma config MCLRE = ON			//Memory Clear Enabled
#pragma config LVP = OFF			//Low-Voltage Programming disabled
#pragma config WDT = OFF			//Watchdog timer off
#pragma config DEBUG = OFF			//Debugging mode off
#pragma config CCP2MX = PORTBE		//CCP2 module MUX set to port B, bit 3 (RB3)

#include <p18f4520.h>				//Includes header file for PIC18F4520
#include <delays.h>					//Includes header file containing delay functions
#include <portb.h>					//Includes header file containing bit addresses for port B
#include <timers.h>					//Includes header file containing timer related functions
#include <usart.h>					//Includes header file containing USART related functions

//Function prototypes
void test_isr (void);
void INT0_ISR (void);
void translate (void);

#pragma code interruption=0x08		//Makes the "interruption" function to start in the 0x08 address

void interruption (void)
	{
		_asm
			GOTO test_isr
		_endasm
	}

#pragma code						//Returns normal code operation

#pragma interrupt test_isr

void test_isr(void)					//Interrupt check routine
{
	if(PORTBbits.RB0 == 0)			//If button is pressed (pull-down circuit, low logic level)
	{
		Nop();						//
		Nop();						//Button debouncing
		Nop();						//
		if(PORTBbits.RB0 == 0) 		//If button is still pressed after debouncing (eliminates mechanical error)
		{
			INT0_ISR();				//Call interrupt service routine (ISR)
		}
	}
	T1CONbits.TMR1ON = 1; 			//Timer1 ON (start counting time for silence)
}

//Global variables
int sil_count, input_count, input_transf, array_pos = 0;
char carac[6];
char letter[2] = " ";

//Main function
void main()
{
	//carac[6] array initialization
	for(array_pos=0;array_pos<=6;array_pos++)
	{
		carac[array_pos] = '0';	
	}
	
	//Variable initialization
	array_pos = 0;			//carac[6] array position
	input_transf = 0;		//input_count transfer variable
	input_count = 0;		//Counter for when the button is pressed
	sil_count = 0;			//Counter for silence
	letter[0] = '0';		//Transmission array containing the letter when translation is done
	
	//Ports setup
	ADCON1 =0x0F; 			//Set Ports as Digital I/O rather than analogue
	TRISD = 0x00; 			//Makes Port D all output (8 bits)
	TRISA = 0x00; 			//Makes Port A all output (8 bits)
	TRISE = 0x00; 			//Makes Port E all output (4 bits)
	LATD = 0x00;  			//Clears Port D
	LATA = 0x00;  			//Clears Port A
	LATE = 0x00;			//Clears Port E
	TRISBbits.TRISB0 = 1;	//Configure PORTB as input
	
	//Interrupt settings
	INTCONbits.INT0IE = 1; 	//Enable INT0 interrupts
	INTCONbits.INT0IF = 0; 	//Clear INT0 Interupt flag
	INTCONbits.GIE = 1; 	//Enable global interrupts
	
	//Timer 1 setup
	T1CONbits.TMR1CS = 0; 	//Timer 1 incrementing in every instruction cycle (Fosc/4)
	T1CONbits.T1CKPS0 = 0;	//
	T1CONbits.T1CKPS1 = 0;	//Timer 1 prescaler set to "00" (PS1:PS0), meaning 1:1 division
	PIE1bits.TMR1IE = 1;	//Enables timer 1 interrupt
	PIR1bits.TMR1IF = 0;	//Set interrupt flag to 0
	T1CONbits.TMR1ON = 0;	//Timer1 OFF (default)
	
	//Timer 2 setup (for use with PWM) 
    T2CONbits.T2CKPS1 = 1;	//Timer 2 prescaler set to "00" (PS1:PS0), meaning 1:1 division
	T2CONbits.T2CKPS0 = 1;
	T2CONbits.TMR2ON = 1;	//Timer 2 ON
	
	//PWM setup
	PR2 = 0xFA;				//PWM period set to 124 (54h)
	CCPR2L = 0x20;	//CCP2 duty cycle set to 54h (decimal = 325, 64,7% duty cycle)
	TRISBbits.TRISB3 = 0;	//Sets port B bit 3 (CCP2A, PWM) as output
	CCP2CON = 0x00;			//CCP2CON<3:0> set to "0000", meaning PWM is off
	
               
	//Main loop
	while(1)
	{
			test_isr();								//Calls interrupt check routine
			if(PORTBbits.RB0 == 0)	INT0_ISR();		//Check whether button is pressed: If yes, go to ISR
			
			input_count = 0;
			if ((input_transf>5) && (input_transf<=114)) 					// 20MHz clock - 76 counter increments for 1s (5 = 66 ms, 114 = 1.5 s)
			{
				if ((input_transf>5)&&(input_transf<=38))	//DOT! Interval: 5(66 ms)~38(500 ms)
				{
					LATD = 0x0F;							//Turns on buzzer
					LATA = 0xFF;							//Turns on Blue LED
  					Delay10KTCYx(200);
					LATD = 0x00;							//Turns off buzzer
					LATA = 0x00;							//Turns off Blue LED
					carac[array_pos] = '.';
					array_pos++;							//Go to next position in carac[6] array
					input_transf = 0;
					sil_count = 0;							//Prevents silence from occurring in the middle of writing in the array
				}
				if (input_transf>38)						//DASH! Interval: 39(aprox. 500 ms)~114(1500 ms)
				{
					CCP2CON = 0x3C;							//CCP2CON<3:0> set to "1100", meaning PWM is ON (Green LED)
					LATD = 0xFF;							//Turns on buzzer and Red LED
  					
  					Delay10KTCYx(400);
					
					CCP2CON = 0x00;							//CCP2CON<3:0> set to "0000", meaning PWM is OFF
					LATD = 0x00;							//Turns off buzzer and Red LED					

					carac[array_pos] = '-';
					array_pos++;							//Go to next position in carac[6] array
					input_transf = 0;
					sil_count = 0;							//Prevents silence from occurring in the middle of writing in the array
				}
			} 
			
			if(input_transf>114)
			{
				//Error!
				LATD = 0x0F;						//Turns on buzzer
				Delay10KTCYx(100);	
				LATD = 0x00;						//Turns off buzzer
				input_transf = 0;
				sil_count = 0;		
			}

			
			if(PIR1bits.TMR1IF)						//If timer 1 overflow occurs
			{
				//Timer1 ISR
				INTCONbits.INT0IE = 0; 				//Disables external interrupt
				sil_count++;						//Increments counter for silence
				PIR1bits.TMR1IF = 0;				//Sets timer 1 interrupt flag to 0 so it can happen again
				WriteTimer1(0x00);					//Resets timer 1
			}
			
			INTCONbits.INT0IE = 1; 					//Re-enables external interrupt
	
			if(PORTBbits.RB0 == 1)					//If button is not pressed
			{
				INTCONbits.INT0IE = 0;					//Disables external interrupt
				if((PIE1bits.TMR1IE == 1)&&(sil_count == 152))	//If silence occurs for 2 seconds
				{
					carac[array_pos] = 's';

					CCP2CON = 0x00;

					LATBbits.LATB3 = 1;					//Turns on Green LED
					LATD = 0xF0;						//Turns on Red LED
					Delay10KTCYx(200);
					LATBbits.LATB3 = 0;					//Turns off Green LED
					LATD = 0x00;						//Turns off Red LED
					sil_count = 0;						//Resets silence counter
					INTCONbits.INT0IE = 1; 				//Re-enables external interrupt
				}
			
				LATD = 0x00;							//Turns off Red LED
				LATBbits.LATB3 = 0;						//Turns off Green LED
			}
			
			if(carac[0] == 's')						//If nothing is inputed in the carac[6] array, transmit a space 
			{
				letter[0] = ' ';
				
				//Transmission (USART with baud rate 19,200)
				OpenUSART(USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 64); 
				putsUSART(letter); 					//Transmits the character

				carac[0] = '0';
				array_pos = 0;
			}
			
			if(carac[0] != 's')						//Only translates if the button is pressed at least once
			{		
				if((array_pos == 6)||(carac[array_pos] == 's')) 	//If array is full or there is a silence
				{	
					translate();					//Call translation function
				}
			}	
	}
}


//Interrupt 0 (Port B, bit 0) Service Routine
void INT0_ISR (void)
{
	INTCONbits.INT0IE = 0; 			// Disable any interrupts from interrupting this function
	
	while(PORTBbits.RB0 == 0)		//While button is pressed
	{
		sil_count = 0;				//Resets silence counter
		PIE1bits.TMR1IE = 1;		//Timer1 ON
		if(PIR1bits.TMR1IF)			//If timer 1 overflows (generates interrupt)
		{
			//Timer1 ISR
			input_count++;
			PIR1bits.TMR1IF = 0;	//Sets timer 1 interrupt flag to 0 so it can happen again
		}	
	}	

	input_transf = input_count;
	WriteTimer1(0x00);				//Resets timer 1
	INTCONbits.INT0IE = 1; 			//Re-enable INT0 interrupts
	INTCONbits.INT0IF = 0; 			//Clear INT0 flag before returning to main program
}


//Translation function
void translate()
{
	array_pos = 0;
	sil_count = 0;				//Prevents silence from occurring mid-translation
	
	switch(carac[array_pos]) 	//array_pos = 0
	{
		case '.': 	array_pos = 1;
					break;
		case '-':	array_pos = 1;
					break;
	}
	
	switch(carac[array_pos]) 	//array_pos = 1
	{
		case '.':	array_pos = 2;
					break;
		case '-':	array_pos = 2;
					break;
		case 's':
		{
			switch(carac[0])
			{
				case '.':	letter[0] = 'E';
							break;
				case '-':	letter[0] = 'T';
							break;
			}
		}
	}
	
	switch(carac[array_pos]) //array_pos = 2
	{
		case '.':	array_pos = 3;
					break;
		case '-':	array_pos = 3;
					break;
		case 's':
		{
			if(carac[0] == '.')
			{
				switch(carac[1])
				{
					case '.':	letter[0] = 'I';
								break;
					case '-':	letter[0] = 'A';
								break;
				}
			}
			if(carac[0] == '-')
			{
				switch(carac[1])
				{
					case '.':	letter[0] = 'N';
								break;
					case '-':	letter[0] = 'M';
								break;
				}
			}
		}	
	}
	switch(carac[array_pos]) //array_pos = 3
	{
		case '.':	array_pos = 4;
					break;
		case '-':	array_pos = 4;
					break;
		case 's':
		{
			if(carac[0] == '.')
			{
				if(carac[1] == '.')
				{
					switch(carac[2])
					{
						case '.':	letter[0] = 'S';
									break;
						case '-':	letter[0] = 'U';
									break;
					}
				}
				if(carac[1] == '-')
				{
					switch(carac[2])
					{
						case '.':	letter[0] = 'R';
									break;
						case '-':	letter[0] = 'W';
									break;
					}
				}
				
			}
			if(carac[0] == '-')
			{
				if(carac[1] == '.')
				{
					switch(carac[2])
					{
						case '.':	letter[0] = 'D';
									break;
						case '-':	letter[0] = 'K';
					}
				}
				if(carac[1] == '-')
				{
					switch(carac[2])
					{
						case '.':	letter[0] = 'G';
									break;
						case '-':	letter[0] = 'O';
									break;
					}
				}
			}
		}
	}
	
	switch(carac[array_pos])	// array_pos = 4
	{
		case '.':	array_pos = 5;
					break;
		case '-':	array_pos = 5;
					break;
		case 's':
		{
			if(carac[0] == '.')
			{
				if(carac[1] == '.')	
				{
					if(carac[2] == '.')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'H';
										break;
							case '-':	letter[0] = 'V';
										break;
						}
					}
					if(carac[2] == '-')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'F';
										break;
						}
					}
				}
				if(carac[1] == '-')
				{
					if(carac[2] == '.')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'L';
										break;
						}
					}
					if(carac[2] == '-')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'P';
										break;
							case '-':	letter[0] = 'J';
										break;
						}
					}
				}
			}
			if(carac[0] == '-')
			{
				if(carac[1] == '.')
				{
					if(carac[2] == '.')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'B';
										break;
							case '-':	letter[0] = 'X';
										break;
						}
					}
					if(carac[2] == '-')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'C';
										break;
							case '-':	letter[0] = 'Y';
										break;
						}
					}
				}
				if(carac[1] == '-')
				{
					if(carac[2] == '.')
					{
						switch(carac[3])
						{
							case '.':	letter[0] = 'Z';
										break;
							case '-':	letter[0] = 'Q';
										break;
						}
					}
				}
			}
		}
	}
	
	switch(carac[array_pos])	// array_pos = 5
	{
		case '-':	
		{
			if(carac[0] == '.')
			{
				if(carac[1] == '-')
				{
					if(carac[2] == '.')
					{
						if(carac[3] == '-')
						{
							if(carac[4] == '.')
							{
								if(carac[5] == '-')
								{	
									letter[0] = '.';
									break;
								}	
							}
						}
					}
				}
			}
			if(carac[0] == '-')
			{
				if(carac[1] == '-')
				{
					if(carac[2] == '.')
					{
						if(carac[3] == '.')
						{
							if(carac[4] == '-')
							{
								if(carac[5] == '-')
								{
									letter[0] = ',';
									break;
								}	
							}
						}
					}
				}
			}
		}	
		case 's':
		{
			if(carac[0] == '.')
			{
				if(carac[1] == '.')	
				{
					if(carac[2] == '.')
					{
						if(carac[3] == '.')
						{
							switch(carac[4])
							{
								case '.':	letter[0] = '5';
											break;
								case '-':	letter[0] = '4';
											break;
							}
						}
						if(carac[3] == '-')
						{
							switch(carac[4])
							{
								case '-':	letter[0] = '3';
											break;
							}
						}
					}
					if(carac[2] == '-')
					{
						if(carac[3] == '-')
						{
							switch(carac[4])
							{
								case '-':	letter[0] = '2';
											break;
							}
						}
					}
				}
				if(carac[1] ==	'-')
				{
					if(carac[2] == '-')
					{
						if(carac[3] == '-')
						{
							switch(carac[4])
							{
								case '-':	letter[0] = '1';
											break;
							}
						}
					}
				}
			}
			if(carac[0] == '-')
			{
				if(carac[1] == '.')	
				{
					if(carac[2] == '.')
					{
						if(carac[3] == '.')
						{
							switch(carac[4])
							{
								case '.':	letter[0] = '6';
											break;
							}
						}
					}
				}
				if(carac[1] ==	'-')
				{
					if(carac[2] == '.')
					{
						if(carac[3] == '.')
						{
							switch(carac[4])
							{
								case '.':	letter[0] = '7';
											break;
							}
						}
					}
					if(carac[2] == '-')
					{
						if(carac[3] == '.')
						{
							switch(carac[4])
							{
								case '.':	letter[0] = '8';
											break;
							}
						}
						if(carac[3] == '-')
						{
							switch(carac[4])
							{
								case '.':	letter[0] = '9';
											break;
								case '-':	letter[0] = '0';
											break;
							}
						}					
					}
				}
			}
		}
	}
	
	sil_count = 0;		//Resets silence counter

	//Transmission (USART with baud rate 19,200)
	OpenUSART(USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 64);  
	putsUSART(letter); 	//Transmits the character 
	
	for(array_pos=0;array_pos<6;array_pos++)
	{
		carac[array_pos] = '0';
	}
	
	array_pos = 0;
}