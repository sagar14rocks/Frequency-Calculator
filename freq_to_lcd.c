/*
 * freq_to_lcd.c
 *
 *  Created on: 10-May-2016
 *      Author: abhas
 */
#include <avr/io.h>
#include <math.h>
#include <util/delay.h>
#include <avr/interrupt.h>
uint8_t o=0;
volatile uint16_t i=1000;
#define lcd_dprt	PORTA	//lcd data port
#define lcd_dddr	DDRA	//lcd data ddr
#define lcd_dpin	PINA	//lcd data pin
#define	lcd_cprt	PORTB	//lcd commands port
#define lcd_cddr	DDRB	//lcd commands ddr
#define	lcd_cpin	PINB	//lcd commands pin
#define	lcd_rs	0			//lcd rs
#define	lcd_rw	1			//lcd rw
#define	lcd_en	3			//lcd en
#define sw_dprt	PORTD	//lcd data port
#define sw_dddr	DDRD	//lcd data ddr
#define sw_dpin	PIND	//lcd data pin
#define 	bit_is_set(sfr, bit)   (_SFR_BYTE(sfr) & _BV(bit))
#define 	bit_is_clear(sfr, bit)   (!(_SFR_BYTE(sfr) & _BV(bit)))
//*********************************************************************************************************************

void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}

 // Converts a given integer x to string str[].  d is the number
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

//*********************************************************************************************************************
void lcdcommand(unsigned char cmnd)
{

	lcd_dprt = cmnd;			//send cmnd to data port
	lcd_cprt &= ~(1<<lcd_rs);	//rs=0 for command
	lcd_cprt &= ~(1<<lcd_rw);	//rw=0 for write
	lcd_cprt |= (1<<lcd_en);	//en=1 for high to low pulse
	_delay_us(1);
	lcd_cprt &= ~(1<<lcd_en);	//en=0 for high to low pulse
	_delay_us(100);
}
//**********************************************************************************************************************
void lcddata(unsigned char data)
{

	lcd_dprt = data;			//send data to data port
	lcd_cprt |= (1<<lcd_rs);	//rs=1 for data
	lcd_cprt &= ~(1<<lcd_rw);	//rw=0 for write
	lcd_cprt |= (1<<lcd_en);	//en=1 for high to low pulse
	_delay_us(1);
	lcd_cprt &= ~(1<<lcd_en);	//en=0 for high to low pulse
	_delay_ms(1);

}
//**********************************************************************************************************************
void lcd_init()
{
	lcd_dddr = 0xFF;
	lcd_cddr = 0xFF;

	lcd_cprt &= ~(1<<lcd_en);	//lcd_en=0
	_delay_us(2000);			//wait fot init
	lcdcommand(0x38);			//init lcd line 1
	//lcdcommand(0x0E);			//display on,cursor on
	lcdcommand(0x0F);
	lcdcommand(0x01);			//clear lcd
	_delay_us(2000);			//wait
	lcdcommand(0x06);			//shift cursor right
	_delay_ms(1);
}
//**********************************************************************************************************************
void lcd_gotoxy(unsigned char x,unsigned char y)
{

	unsigned char firstcharadr[] = {0x80,0xC0,0x94,0xD4};	//line select 0x80 for line 1
	lcdcommand(firstcharadr[y-1] + x - 1);
	_delay_ms(1);

}
//**********************************************************************************************************************
void lcd_print( char * str )
{
	unsigned char y = 0;
	while(str[y]!=0)
	{	//_delay_ms(1);
		lcddata(str[y]);
		y++;

	}

}
//**********************************************************************************************************************

void send_data(uint16_t j)
{
	uint16_t temp=j;
	uint8_t a[4];
	char t[3];
	a[0] = temp/1000;
	temp = temp%1000;
	a[1] = temp/100;
	temp = temp%100;
	a[2] = temp/10;
	a[3] = temp%10;
	for(int x=0;x<=3; x++)
	{
	  t[x] = 0x30 | a[x];  //it is better than addition (+)
	  lcddata(t[x]);
	}

}


unsigned int overflow_counter, rising_edge, falling_edge, pulse_width, last_val, first_val;
unsigned int TOP = 65535;
unsigned long int clock = 11059200;
float freq;
char str[6];
//This interrupt service routine calculates the pulse width of a square wave on the ICP pin
//and displays that in a certain form on PORT D

ISR(TIMER1_CAPT_vect)
{
	int ipart;
	// If ICP pin is set, there was a rising edge else if its low there must have been a falling edge /
	if (bit_is_set(PIND,6))
	{
		first_val = ICR1;
		o++;
		if(o>=2)
				{
					o=0;
					pulse_width =last_val - first_val + TOP*overflow_counter;
							freq=clock/(TOP-pulse_width);
							ipart=(int)freq;
							//ftoa(freq,str,2);
							lcdcommand(0x01);
							lcdcommand(0x80);
							lcd_print("  frequency=");
							lcdcommand(0xc0);
							send_data(ipart);
							//lcd_print(str);
							lcd_print("Hz");
							last_val=first_val;
							_delay_ms(1000);
				}
	


		overflow_counter = 0;
	}
	
}


ISR(TIMER1_OVF_vect)
{

	overflow_counter++; //increment counter when overflow occurs

}


int main(void)
{


	DDRD = 0;// ICP pin as input which is pin 0 on port B/
	DDRA = 0XFF; // ALL PINS on port D are set as output.
	DDRB = 0XFF;
	sei();//enable global interrupt

	//setting up Timer control register 1

	//TOP VALUE IS 0XFFFF; (NORMAL MODE)
	TCCR1A = 0;
	TCCR1B = 0;


	//SETS PRESCALER ON 1
	TCCR1B |= (1<<CS10);
	TCCR1B &= ~(1<<CS11) & ~(1<<CS12);

	// Enable Input noise canceller and capture time on rising edge
	TCCR1B |=  (1<<ICES1) | (1<<ICNC1) ;

	TIMSK = 0;
	TIMSK |= (1<<TICIE1); //Enable Input Capture Interrupt
	TIMSK |= (1<<TOIE1); //Enable timer overflow interrupt
	lcd_init();
		//lcd_gotoxy(1,1);
		lcdcommand(0x80);
		lcd_print("Hello World");
		_delay_ms(1000);
		lcdcommand(0x01);
		lcdcommand(0x80);
		lcd_print("  Init freq calc");
	while(1)
	{

	}
	return 0;
}


