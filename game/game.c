#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6

char player1Score = '0';
char player2Score = '0';
short score = 1;

u_int bgColor = COLOR_VIOLET;
int redrawScreen = 1;
Region fieldFence;
Region fence = {{0,LONG_EDGE_PIXELS}, {SHORT_EDGE_PIXELS, LONG_EDGE_PIXELS}};

AbRect paddle = {abRectGetBounds, abRectCheck, {15,3}}; 
AbRect middleLine = {abRectGetBounds, abRectCheck, {61, 0}}; 
AbRectOutline fieldOutline = {
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 2, screenHeight/2 - 6}
};

Layer fieldLayer = {
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2 -3},
  {0,0}, {0,0},	
  COLOR_WHITE,
  0
};
Layer layer4 = {
  (AbShape *)&middleLine,
  {(screenWidth/2 ), (screenHeight/2 - 3)}, 
  {0,0}, {0,0},	
  COLOR_WHITE,
  &fieldLayer,
};		     
Layer layer3 = {
  (AbShape *)&paddle,
  {(screenWidth/2), (screenHeight/2)-70},
  {0,0}, {52,10},
  COLOR_WHITE,
  &layer4,
};
Layer layer2 = {
  (AbShape *)&circle6,
  {screenWidth/2, (screenHeight/2)},
  {0,0}, {0,0},
  COLOR_GREEN,
  &layer3,
};
Layer layer0 = {
  (AbShape *)&paddle,
  {(screenWidth/2), (screenHeight/2)+64}, 
  {0,0}, {52,144},
  COLOR_WHITE,
  &layer2,
};

typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

MovLayer ml1 = { &layer2, {5,5}, 0 };
MovLayer ml2 = { &layer0, {5,5}, 0 };
MovLayer ml3 = { &layer3, {5,5}, 0 };

int movLayerDraw(MovLayer *movLayers, Layer *layers){
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {

    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], bounds.botRight.axes[0], bounds.botRight.axes[1]);

    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++){
	      Vec2 pixelPos = {col, row};
	      u_int color = bgColor;
	      Layer *probeLayer;
	      for (probeLayer = layers; probeLayer;probeLayer = probeLayer->next) {
	        if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	          color = probeLayer->color;
	          break; 
	        }
	      }
	      lcd_writeColor(color); 
      } 
    }
  }
}	  

void gameOver(char x){
  if(x ==0){
	  bgColor = COLOR_WHITE;
	  layerDraw(&layer0);
	  drawString5x7(screenWidth/4, screenHeight/2, "Game Over", COLOR_BLUE, COLOR_WHITE);
	  drawString5x7(screenWidth / 4, screenHeight / 2 + 10, "Player 1 Wins", COLOR_VIOLET, COLOR_WHITE);
	  int redrawScreen = 1;
	  int count = 0;
	  while(++count != 50){}
  }
  else{
	  bgColor = COLOR_WHITE;
	  layerDraw(&layer0);
	  drawString5x7(screenWidth/4, screenHeight/2, "Game Over", COLOR_BLUE, COLOR_WHITE);
	  drawString5x7(screenWidth / 4, screenHeight / 2 + 10, "Player 2 Wins", COLOR_VIOLET, COLOR_WHITE);
	  int redrawScreen = 1;
	  int count = 0;
	  while(++count != 50){}
  }
}

void newGame(){
	bgColor = COLOR_VIOLET;
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
	    }
	    
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

	    else if (ml->layer->posNext.axes[1] == 10){
	      ml2->layer->color = COLOR_RED;
	      player1Score ++;
	      drawChar5x7(52,152, player1Score, COLOR_YELLOW, COLOR_VIOLET);
	      newPos.axes[0] = screenWidth/2;
	      newPos.axes[1] = (screenHeight/2);
	      score = 1;
	      ml->velocity.axes[0] = 5;
	      ml->layer->posNext = newPos;
	      int redrawScreen = 1;
	    }
	    
	    else if (ml->layer->posNext.axes[1] == 145){
	      ml1->layer->color = COLOR_RED;
	      player2Score ++;
	      drawChar5x7(120,152, player2Score, COLOR_GREEN, COLOR_VIOLET);	   
	      newPos.axes[0] = screenWidth/2;
	      newPos.axes[1] = (screenHeight/2);
	      score = 1;
	      ml->velocity.axes[0] = 5;
	      ml->layer->posNext = newPos;
	      int redrawScreen = 1;
	    }
      int redrawScreen = 1;
      if(score != 1) ml->layer->posNext = newPos;
    }
  }
}

void main(){
  P1DIR |= GREEN_LED;	
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  buzzer_init();
  p2sw_init(15);
  
  if(score == 1)buzzer_set_period(0);

  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence);
  drawString5x7(3, 152, "Player1:", COLOR_YELLOW, COLOR_VIOLET);
  drawString5x7(72, 152, "Player2:", COLOR_GREEN, COLOR_VIOLET);
	
  enableWDTInterrupts();
  or_sr(0x8);

  for(;;) {
    while (!redrawScreen){
      P1OUT &= ~GREEN_LED;
      or_sr(0x10);
    }
    P1OUT |= GREEN_LED;
    redrawScreen = 0;
 
    movLayerDraw(&ml1, &layer0);
    movLayerDraw(&ml2, &layer0);
    movLayerDraw(&ml3, &layer0);
  }
}

void wdt_c_handler(){
  static short count = 0;
  P1OUT |= GREEN_LED;
  count ++;
  u_int switches = p2sw_read();
	
  if(player1Score == '9'){
	  char new_game = 1;
	  gameOver(0);
	  while (new_game){
	  	if(switches){
			newGame();
			main();
		}
	  }
  }
  else if(player2Score == '9'){
	  char new_game = 1;
	  gameOver(1);
	  while (new_game){
	  	if(switches){
			newGame();
			main();
		}
	  }
  }
	
  if (count == 15) {
	  
    mlAdvance(&ml1, &ml2, &ml3,  &fieldFence);
	  
    if(!(switches & (1 << 0))){
	    if(ml2.layer->posNext.axes[0] >= 27){
		    ml2.layer->posNext.axes[0] -= 5;
		    redrawScreen = 1;
		    score = 0;
	    }
    }

    else if(!(switches & (1 << 1))){
	    if(ml2.layer->posNext.axes[0] <= 102){	
		    ml2.layer->posNext.axes[0] += 5;
		    redrawScreen = 1;
		    score = 0;
	    }
    }

    else if(!(switches & (1 << 2))){
	    if(ml3.layer->posNext.axes[0] >= 26){
		    ml3.layer->posNext.axes[0] -= 5;
		    redrawScreen = 1;
		    score = 0;
	    }
    }

    else if(!(switches & (1 << 3))){
	    if(ml3.layer->posNext.axes[0] <= 102){
		    ml3.layer->posNext.axes[0] += 5;
		    redrawScreen = 1;
		    score = 0;
	    }
    }

    redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;
}
