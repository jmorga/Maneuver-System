/*
 * Maneuver System.cpp
 *
 * Created: 2/18/2020 3:25:51 PM
 * Author : Joseph Morga
 */ 

#define F_CPU 16000000UL  //16MHz
#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)

//Photoresistors 0 -> 3
#define R0 0    //Top
#define R1 1	//Bottom
#define R2 2	//Left
#define R3 3	//Right

#define frequency 10

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

//UART for displaying the values from the photoresistors *Test Purposes Only*
void uart_Init();
void uart_Transmit(char data[]);

//To convert values from ADC to string for the UART
void convertToString(int16_t value, char word[]);
char getChar(int  digit);

//Initialization functions
void adc_Init();
void dac_Init();
void timer_Init();
void inputOutput_Init();
uint16_t adc_read(uint8_t channel);



//Moving functions
void startManeuvering(); //Read voltage values from the 4 voltage dividers
void moveLinearActuator();
void moveStepperMotor();

char value[7];
int notManeuvering = 1;

int main(void)
{
	uart_Init();
	adc_Init();
	timer_Init();
	inputOutput_Init();	

	sei();

	while (1) {
// 		if(!(PINB & (1<<PINB7))) // PINB7 is low (Button has been pressed)
// 		{
// 			while(!(PINB & (1<<PINB7)));
// 			
// 			startManeuvering();
// 		}
	}
}

void dac_Init()
{
	DDRB |= (1<<DDB0)|(1<<DDB1);
}

void adc_Init(){
	//select Vcc and select ADC1 as input
	ADMUX = (1<<REFS0);
	
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

void timer_Init(){
	OCR1A = 0x3D08;

	TCCR1B |= (1 << WGM12);
	// Mode 4, CTC on OCR1A

	TIMSK1 |= (1 << OCIE1A);
	//Set interrupt on compare match

	TCCR1B |= (1 << CS12) | (1 << CS10);
	// set prescaler to 1024 and start the timer
}

ISR (TIMER1_COMPA_vect)
{
	if(notManeuvering){
		notManeuvering = 0;
		PINB |= (1<<PINB5); //toggle LED
		startManeuvering();
		PINB |= (1<<PINB5); //toggle LED
		notManeuvering = 1;
	}
	//We done ;)
}

void inputOutput_Init(){
	DDRB |= (1 << DDRB5);  //Set portB 5 as output LED
	
	//Output pinD 2 and 3 for linear actuator and pinD 4 and 5 for stepper motor
	DDRD |= (1 << DDD2) | (1 << DDD3) | (1 << DDD4)| (1 << DDD5);
	
	DDRB &= ~(1<<DDRB7);   //Set portB 7 as input (Button input)
	
	//PinC 0, 1, 2 and 3 as input for voltage values from photoresistors
	DDRC &= ~((1<<DDRC0) | (1 << DDRC1) | (1 << DDRC2) | (1 << DDRC3));
	
	//Set pins D 2 and 3 to power off the linear actuator by default
	PORTD |= (1 << PORTD3) | (1 << PORTD2);
}

void startManeuvering(){
	moveStepperMotor();
	moveLinearActuator();
}

void moveLinearActuator(){
	
	int16_t v0;
	int16_t v1;
	int16_t difference;
// 	int count = 0;
// 	int displaceTimer = 0;
	
	v0 = adc_read(R0);              //Read voltage value v0 and v1
	
   	convertToString(v0, value);     //Used to convert the value into
   	uart_Transmit(value);			//a string and to display it
	
	v1 = adc_read(R1);
   	convertToString(v1, value);     //Used to convert the value into
   	uart_Transmit(value);			//a string and to display it
	
	difference = v0 - v1;           //Take difference
	
	if(difference > -50) PORTD &= ~(1 << PORTD3);        //Send signal to start retracting
	else if(difference < 50) PORTD &= ~(1 << PORTD2);   //Send signal to start expanding
	
	while(difference < -50 || difference > 50){        //True while the difference is outside the [-50, 50] range
		
	/*	if(displaceTimer == 15) break; */ //After 20 seconds leave the loop
		
		v0 = adc_read(R0);
		v1 = adc_read(R1);			                   //Continue reading voltage values and take difference
		difference = v0 - v1;
		
		//count++;
		
// 		if(count == 4286){     //4286 is about 1 second
// 			 displaceTimer++;
// 			 count = 0;
// 		}
	}
	
	PORTD |= (1 << PORTD3) | (1 << PORTD2);  //Cut the power of the linear actuator
}

void moveStepperMotor(){
	int16_t v2;
	int16_t v3;
	int16_t difference;
	
	v2 = adc_read(R2);              //Read voltage value v2
	
// 	convertToString(v2, value);     //Used to convert the value into
//  	uart_Transmit(value);		//a string and to display it
	
	v3 = adc_read(R3);				//Read voltage value v3
//  	convertToString(v3, value);
//  	uart_Transmit(value);
	
	difference = v2 - v3;           //Take difference
	
//  	convertToString(difference, value);
//  	uart_Transmit(value);
	
	if(difference > 50) PORTD |= (1 << PORTD5);        //Rotate Clockwise
	else if(difference < -50) PORTD &= ~(1 << PORTD5); //Rotate Counter-Clockwise
	
	while(difference < -50 || difference > 50){//True while the difference is outside the [-50, 50] range
		v2 = adc_read(R2);
		v3 = adc_read(R3);			           //Continue reading voltage values and take difference
		difference = v2 - v3;
		
		//Generating square wave
		PORTD |= (1 << PORTD4);
		_delay_ms(frequency);
		PORTD &= ~(1 << PORTD4);
		_delay_ms(frequency);
	}
}

//converts analog input and converts it into a digital value in the range of 0 - 1024,
//where 0 represents 0v and 1024 5v
uint16_t adc_read(uint8_t channel){
	
	channel &= 0b00000111;
	
	ADMUX = (ADMUX & 0xF8) | channel;
	
	ADCSRA |= (1<<ADSC);
	
	while(ADCSRA & (1<<ADSC));
	
	return ADC;
}

//Used to display data in the terminal for testing purposes
void uart_Transmit(char data[]){
	for(int i = 0; i < 6; i++){
		while(!(UCSR0A & (1<<UDRE0)));      //wait for register to be free
		UDR0 = data[i];
	}
	
	while(!(UCSR0A & (1<<UDRE0))); 
	UDR0 = ' ';
	
	for(int i = 0; i < 6; i++) value[i] = ' ';
}

void uart_Init(){
	UBRR0H = (BAUDRATE>>8);				 //shift register to the right by 8 bits
	UBRR0L = BAUDRATE;					 //set baud rate
	UCSR0B |= (1<<TXEN0) | (1<<RXEN0);   //enable receiver and transmitter
	UCSR0C |= (1<<UCSZ00) | (1<<UCSZ01); //8 bit data format
}

void convertToString(int16_t voltage, char word[])
{
	double number = voltage;
	int divisionCount = 0;
	int digit;
	
	if(number < 0) number *= -1;
	
	while(number >= 1){
		 number = number / 10;
		 divisionCount++;
	}
	for(int i = 0; i < divisionCount && i < 6; i++){
		
		number = number * 10;
		digit = (int)number;
		number = number - digit;
		word[i] = getChar(digit);
	}
	
	word[6] = '\0';
}

char getChar(int digit){
	
	switch(digit)
	{
		case 1: return '1';
		case 2: return '2';
		case 3: return '3';
		case 4: return '4';
		case 5: return '5';
		case 6: return '6';
		case 7: return '7';
		case 8: return '8';
		case 9: return '9';
		default: return '0';
	}
}