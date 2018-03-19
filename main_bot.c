//Adam Tanana Trader bot COMP1511 Introduction to Programming

#include "trader_bot.h"
#include <stdio.h>
#include "utils.h"
#include <stdlib.h>
#include <math.h>

/**
 Function called from testing program
 Save (action, value) pair as return value
 Actions can be found in trader_bot.h
*/
void get_action(struct bot *b, int *action, int *n) {
	if(b->cargo != NULL && b->cargo->quantity > 0) { //If we have cargo in our inventory then sell it!
		int amount = b->cargo->quantity; //calculate amount we can sell
		struct location *buyer = findBestBuyer(b, b->cargo->commodity, &amount, b->location);	
		int distance = calculateDistance(b->location, buyer); //which way to go to get there
		
		if(distance == 0) { //if we here then sell it 
			if(amount > 0 && buyer->commodity == b->cargo->commodity && buyer->quantity > 0 && botCount(buyer) < buyer->quantity) {
				*action = ACTION_SELL;
				*n = amount;
			}else {
				dump(b, action,n); //what we can't sellit? guess we must dump it
			}
		} else { //oh ok we aren't at the right place, thats a good start
            if(b->fuel - abs(distance) < abs(nearestFuel(buyer)) && abs(nearestFuel(buyer)) != MAX_LOCATIONS) {
				forceRefuel(b, action, n); //Do we have enough 7/11 Supreme diesel to power our dual turbo gtr?
                return;
            }        
			*action = ACTION_MOVE;
			*n = distance; 
		}
	} else { //so we dont hav any items, lets buy some
		int amount = 0;
		int refuel = 0;
		struct location *moveTo = findBestOption(b, &amount, &refuel);
		
		if(refuel) {
			forceRefuel(b, action, n); //lets refuel if we need to
			return;
		}
		if(moveTo == NULL) { //no good place to buy, lets stay still forever
			*action = ACTION_MOVE;
			*n = 0;
			return;
		}

		int distance = calculateDistance(b->location, moveTo);

		if(distance == 0) { //same as above, if we are there buy, otherwise move
			*action = ACTION_BUY;
			*n = amount;
		} else {
			*action = ACTION_MOVE;
			*n = distance;
		}
	
	}
} 

char *get_bot_name() {
    return "No thankyou"; 
} 
