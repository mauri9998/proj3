#include <msp430.h>
#include "buzzer.h"
static unsigned int period = 1000;
static signed int rate = 200;	

#define MIN_PERIOD 1000
#define MAX_PERIOD 4000

double notes[] = {1898, 0, 1898, 0, 1898, 2394, 2134, 1898, 2134, 1898};
long tempos[] = {25, 0, 25, 0, 75, 75, 75, 50, 25, 150}; 

void buzzer_init()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */
}

void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 5;		/* one half cycle */
}

void playSong(){
	int i = 0;
	long n;
    
	while(notes[i] != '\0'){
		n = 0;
		buzzer_set_period(notes[i]);
 		while(++n < tempos[i]){}
		i++;
	}
}
