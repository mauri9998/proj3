#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6

short goal = 1; // checks if any player has made a goal
char player1Score = '0'; // player1 score tracker
char player2Score = '0'; // player2 score tracker

AbRect paddle = {abRectGetBounds, abRectCheck, {15,3}}; /**< 15x3 rectangle */
AbRect middleLine = {abRectGetBounds, abRectCheck, {61, 0}}; // horizontal line
AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 2, screenHeight/2 - 6}
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2 -3},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  0
};
Layer layer4 = {		/**< Layer with a horizontal line */
  (AbShape *)&middleLine,
  {(screenWidth/2 ), (screenHeight/2 - 3)}, /**< Middle field divider */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &fieldLayer,
};		     
Layer layer3 = {		/**< Layer with a white paddle */
  (AbShape *)&paddle,
  {(screenWidth/2), (screenHeight/2)-70}, /** Top of Screen */
  {0,0}, {52,10},				    /* last & next pos */
  COLOR_WHITE,
  &layer4,
};
Layer layer2 = {		/**< Layer with a green circle */
  (AbShape *)&circle6,
  {screenWidth/2, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &layer3,
};
Layer layer0 = {		/**< Layer with a white paddle */
  (AbShape *)&paddle,
  {(screenWidth/2), (screenHeight/2)+64}, /**< Bottom of Screen */
  {0,0}, {52,144},				    /* last & next pos */
  COLOR_WHITE,
  &layer2,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml1 = { &layer2, {5,5}, 0 };// Ball Layer ml1
MovLayer ml2 = { &layer0, {5,5}, 0 };// Bottom paddle Layer
MovLayer ml3 = { &layer3, {5,5}, 0 };// Upper paddle Layer

int movLayerDraw(MovLayer *movLayers, Layer *layers){
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */

    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], bounds.botRight.axes[0], bounds.botRight.axes[1]);

    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++){
	      Vec2 pixelPos = {col, row};
	      u_int color = bgColor;
	      Layer *probeLayer;
	      for (probeLayer = layers; probeLayer;probeLayer = probeLayer->next) { /* probe all layers, in order */
	        if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	          color = probeLayer->color;
	          break; 
	        } /* if probe check */
	      } // for checking all layers at col, row
	      lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

Region fence = {{0,LONG_EDGE_PIXELS}, {SHORT_EDGE_PIXELS, LONG_EDGE_PIXELS}}; /**< Create a fence region */

void gameOver(char x){
  if(x ==0){
    bgColor = COLOR_WHITE;
    layerDraw(&layer0);
    drawString5x7(screenWidth/2, screenHeight/2, "Game Over", COLOR_BLUE, COLOR_BLACK);
    int redrawScreen = 1;
  }
  else{
    bgColor = COLOR_WHITE;
    layerDraw(&layer0);
    drawString5x7(screenWidth/2, screenHeight/2, "Game Over", COLOR_BLUE, COLOR_BLACK);
    int redrawScreen = 1;
  }
}

void newGame(){
	bgColor = COLOR_BLACK;
    	layerDraw(&layer0);
	player1Score = '0';
	player2Score = '0';
	int redrawScreen = 1;
}

void mlAdvance(MovLayer *ml, MovLayer *ml1, MovLayer *ml2, Region *fence){
  Vec2 newPos;
  
  u_char axis;
  Region shapeBoundary;

  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	      buzzer_set_period(0);
	      newPos.axes[axis] += (2*velocity);
      }/**< if outside of fence */

      // Check if ball has collided with paddle
      if( (ml->layer->posNext.axes[1] >= 134) && (ml->layer->posNext.axes[0] <=  ml1->layer->posNext.axes[0] + 18 && ml->layer->posNext.axes[0] >= ml1->layer->posNext.axes[0] - 18)){
	      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	      ml1->layer->color = COLOR_YELLOW;
	      ml2->layer->color = COLOR_WHITE;
	      ml->layer->color = COLOR_YELLOW;
	      ml->velocity.axes[0] += 1;
	      newPos.axes[axis] += (2*velocity);
	      buzzer_set_period(1000);
	      int redrawScreen = 1;
	    }

      // Check if ball has collided with paddle
      else if( (ml->layer->posNext.axes[1] <= 21) && (ml->layer->posNext.axes[0] <=  ml2->layer->posNext.axes[0] + 18 && ml->layer->posNext.axes[0] >= ml2->layer->posNext.axes[0] - 18)){
	      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	      ml2->layer->color = COLOR_GREEN;
	      ml1->layer->color = COLOR_WHITE;
	      ml->layer->color = COLOR_GREEN;
	      ml->velocity.axes[0] += 1;
	      newPos.axes[axis] += (2*velocity);
	      buzzer_set_period(5000);
	      int redrawScreen = 1;
	    }

      // Check if ball has collided with upper fence
	    else if (ml->layer->posNext.axes[1] == 10){
	      ml2->layer->color = COLOR_RED;
	      player1Score ++;
	      drawString5x7(3, 152, "Player1:", COLOR_YELLOW, COLOR_BLACK);
	      drawChar5x7(52,152, player1Score, COLOR_YELLOW, COLOR_BLACK);
	      newPos.axes[0] = screenWidth/2;
	      newPos.axes[1] = (screenHeight/2);
	      goal = 1;
	      ml->velocity.axes[0] = 5;
	      ml->layer->posNext = newPos;
	      int redrawScreen = 1;
      }

      // Check if ball has collided with lower fence
	    else if (ml->layer->posNext.axes[1] == 145){
	      ml1->layer->color = COLOR_RED;
	      player2Score ++;
	      drawString5x7(72, 152, "Player2:", COLOR_GREEN, COLOR_BLACK);
	      drawChar5x7(120,152, player2Score, COLOR_GREEN, COLOR_BLACK);	   
	      newPos.axes[0] = screenWidth/2;
	      newPos.axes[1] = (screenHeight/2);
	      goal = 1;
	      ml->velocity.axes[0] = 5;
	      ml->layer->posNext = newPos;
	      int redrawScreen = 1;
	    }

      int redrawScreen = 1;
      //If no player goal keep on updating the ball's position
      if(goal != 1) ml->layer->posNext =  newPos;
    }/**< for axis */
  }/**< for ml */
}

u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */
Region fieldFence;		/**< fence around playing field  */


void main(){
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  buzzer_init();
  p2sw_init(15);
  
  if(goal == 1)buzzer_set_period(0);

  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence);

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) {
    while (!redrawScreen){ /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;
    /**< Green led on when CPU on */
    redrawScreen = 0;
 
    movLayerDraw(&ml1, &layer0);
    movLayerDraw(&ml2, &layer0);
    movLayerDraw(&ml3, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec 
 *  Also has logic statements implemented for switch handlers on SW2-SW5 buttons
 *  SW2-SW3 will move the bottom paddle while SW4-SW5 will move the upper paddle
 */

void wdt_c_handler(){
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  u_int switches = p2sw_read();
	
  if(player1Score == '9'){
	  char new_game = 1;
	  gameOver(0);
	  while (new_game){
		count ++;
	  	if(switches && count == 15){
			newGame();
			main();
		}
	  }
  }
  else if(player2Score == '9'){
	  char new_game = 1;
	  gameOver(0);
	  while (new_game){
		count ++;
	  	if(switches && count == 15){
			newGame();
			main();
		}
	  }
  }
	
  if (count == 15) {
	  
    mlAdvance(&ml1, &ml2, &ml3,  &fieldFence);

    if(!(switches & (1 << 1))){
	    if(ml2.layer->posNext.axes[0] <= 102){	
	      ml2.layer->posNext.axes[0] += 5;
	      bgColor = COLOR_BLACK;
	      redrawScreen = 1;
	      goal = 0;
	    }
    }

    else if(!(switches & (1 << 0))){
      if(ml2.layer->posNext.axes[0] >= 27){
              ml2.layer->posNext.axes[0] -= 5;
	      bgColor = COLOR_BLACK;
	      redrawScreen = 1;
	      goal = 0;
      }
    }

    else if(!(switches & (1 << 2))){
       if(ml3.layer->posNext.axes[0] >= 26){
	      ml3.layer->posNext.axes[0] -= 5;
	      bgColor = COLOR_BLACK;
              redrawScreen = 1;
              goal = 0;
      }
    }

    else if(!(switches & (1 << 3))){
      if(ml3.layer->posNext.axes[0] <= 102){
        ml3.layer->posNext.axes[0] += 5;
	bgColor = COLOR_BLACK;
        redrawScreen = 1;
        goal = 0;
      }
    }

    redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
