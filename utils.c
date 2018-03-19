#include "trader_bot.h"
#include <stdio.h>
#include "utils.h"
#include <stdlib.h>


//Count all locations in world
int locationCount(struct location *start) {
	struct location *current = start;

	int count = 0;

	do {
		count++;
		current = current->next;
	} while(current != start); 

	return count;
}

//Count all bots at a certain location
int botCount(struct location* loc) {
	int count = 0;
	struct bot_list *bots = loc->bots;

	if(bots == NULL) { // no bots
		return 0;
	}

	do {
		count++;
		bots = bots->next;
	} while(bots != NULL);

	return count;
}

//Find the nearest dump to a location 
struct location *nearestDump(struct location *start, int *distance) {
	int worldSize = locationCount(start);
	struct location* pos; //save the positive and negative closest places
	struct location* neg;

	struct location *current= start; 
	int posdistance = MAX_LOCATIONS, negdistance = MAX_LOCATIONS;

	for(int i = 0; i < worldSize ; i++) { //Loop through both postive and negative directions
		if(current->type == LOCATION_DUMP) {
			if(i < posdistance) {
				pos = current;
				posdistance = i;
			}
		}
		current = current->next;
	}
	current = start;

	for(int i = 0; i < worldSize ; i++) {
		if(current->type == LOCATION_DUMP) {
			if(i < negdistance) {
				neg = current;
				negdistance = i;
			}
		}
	current = current->previous;
	}	
	if(negdistance < posdistance) {
		*distance = -negdistance;
		return neg;
	}
	*distance = posdistance;
	return pos;
}

//Calculate total fuel in world
int totalFuelInWorld(struct bot* b) {
	struct location *current = b->location;
	int total = 0;
	do{
		if(current->type == LOCATION_PETROL_STATION && current->quantity > 5 && botCount(current) < current->quantity) {
			total += current->quantity;
		}
		current =  current->next;	
	}while(current != b->location);
	return total;
}

//Calculate average fuel price in world
int averageFuel(struct bot* b) {
	struct location *current = b->location;
	int total = 0;
	int count = 0;

	do{
		if(current->type == LOCATION_PETROL_STATION && current->quantity > 5 && botCount(current) < current->quantity) {
			total += current->price;
			count++;
		}
		current =  current->next;	
	}while(current != b->location);
	if(count == 0) return 0;

	return total / count;
}

//Proceed to dump move
void dump(struct bot* b, int *action, int *n) {
	if(b->location->type == LOCATION_DUMP) {
		*action = ACTION_DUMP;
		*n = b->cargo->quantity;
	} else {
		int nearest = 0;
		struct location* dump = nearestDump(b->location, &nearest);

		if(b->fuel - abs(nearest) < abs(nearestFuel(dump))) { //if we cant reach the dump
			forceRefuel(b, action, n);
			return;
		}
		*action = ACTION_MOVE;
		*n = nearest;
	}
}

//Calculate distance to nearest fuel
int nearestFuel(struct location *start) {
	int worldSize = locationCount(start);
	
	struct location *current= start;
	int posdistance = MAX_LOCATIONS, negdistance = MAX_LOCATIONS;
	for(int i = 0; i < worldSize ; i++) {
		if(current->type == LOCATION_PETROL_STATION && current->quantity > 5 && botCount(current) < current->quantity) {
			if(i < posdistance) {
				posdistance = i;
			}
		}
		current = current->next;
	}
	current = start;

	for(int i = 0; i < worldSize ; i++) {
		if(current->type == LOCATION_PETROL_STATION && current->quantity > 5 && botCount(current) < current->quantity) {
			if(i < negdistance) {
				negdistance = i;
			}
		}
	current = current->previous;
	}	
	if(negdistance < posdistance) {
		return -negdistance;
	}
	return posdistance;
}

//Calculate distance from a -> b
int calculateDistance(struct location *start, struct location *end) {
	if(start==end) return 0;

	int worldSize = locationCount(start);
	struct location *current = start;
	int posdistance = MAX_LOCATIONS, negdistance = MAX_LOCATIONS;

	for(int i = 0; i < worldSize/2+1 ; i++) {
		if(current == end) {
			if(i < posdistance) {
				posdistance = i;
			}
		}
		current = current->next;
	}
	current = start;

	for(int i = 0; i < worldSize/2+1 ; i++) {
		if(current == end) {
			if(i < negdistance) {
				negdistance = i;
			}
		}
		current = current->previous;
	}	
	if(negdistance < posdistance) {
		return -negdistance;
	}
	return posdistance;
}

//Like dump but refuel
void forceRefuel(struct bot *b, int *action, int *n) {
	if(b->location->type == LOCATION_PETROL_STATION && b->location->quantity > 5 && botCount(b->location) < b->location->quantity) {
		*action = ACTION_BUY;
		*n = (b->fuel_tank_capacity - b->fuel);
	} else {
		int nearestPetrol = nearestFuel(b->location);
		*action = ACTION_MOVE;
		*n = nearestPetrol;
	}
}

//find all possible buyers for a commodity
struct location **findBuyers(struct bot *b, struct commodity *commodity) {
	struct location *start = b->location;
	struct location *current = start;
	struct location **sellerArray = calloc(MAX_LOCATIONS, sizeof(struct location*));

	int counter = 0;
	do {
		if(current->commodity == commodity && current->type == LOCATION_BUYER) {
			if(current->quantity > 0 && botCount(current) < current->quantity) {
				sellerArray[counter++] = current;
			}
		}
		current = current->next;
	} while(current != start);


	sellerArray[counter] = NULL; //set last value to null;
	return sellerArray;
}

//find BEST buyer from above functon
struct location *findBestBuyer(struct bot *b, struct commodity* commodity, int* amount, struct location* loc) {
	struct location **sellerArray = findBuyers(b, commodity);
	int counter = 0;
	int bestProfit = 0;
	struct location* bestSeller = loc;
	int bestAmount = 0;

	while(sellerArray[counter] != NULL && counter < MAX_LOCATIONS) {
		struct location *seller = sellerArray[counter++];
		int tempAmount = 0;

		if(seller->quantity < *amount) {
			tempAmount = seller->quantity;
		} else {
			tempAmount = *amount;
		}

		int distance = calculateDistance(loc, seller);

		int totalProfit = tempAmount * (seller->price);// - (averageFuel(b) * distance);

		if(totalProfit > bestProfit) {
			bestProfit = totalProfit;
			bestSeller = seller;
			bestAmount = tempAmount;
		}
	}

	*amount = bestAmount;
	free(sellerArray);
//	int a = bestSeller->price;
	return bestSeller;
}


/**
	Find the best combination of buy/sell routine and return the location to move to
*/
struct location *findBestOption(struct bot *b, int *amt, int *refuel) {
	struct location *start = b->location;
	struct location *current = start;

	int profit = 0;
	struct location *bestDeal = start;
	int fuelNeeded = 0;

	do { //Loop thru all sellers in world
		if(current->type == LOCATION_SELLER) {
			struct commodity *commodity = current->commodity;

			//Calculate maximum amount you can carry
			int amount = current->quantity;

			if(b->maximum_cargo_volume / commodity->volume < amount) {
				amount = b->maximum_cargo_volume /  commodity->volume;
			}

			if(b->maximum_cargo_weight / commodity->weight < amount) {
				amount = b->maximum_cargo_weight / commodity->weight;
			}

			if((b->cash - 1000) / (current->price) < amount) { //keep $1000 INCASE I NEED REFUEL *SADFACE*
				amount = (b->cash - 1000) / (current->price);
			}

			if(amount == 0) {
				current = current->next;
				continue;
			}

			//find best buyer combination to the current location
			struct location *buyer = findBestBuyer(b, commodity, &amount, current);	

			if(buyer == current) {
				current = current->next;
				continue;
			}
			int distanceA = abs(calculateDistance(b->location, current));
			int distanceB = abs(calculateDistance(current, buyer));

			//Calculate total profit from this combination
			int totalProfit = (amount * (buyer->price - current->price));// - (averageFuel(b) * (distanceA + distanceB));
			
			if(totalProfit > profit) { //Save the best profitable thingo
				int turnsRequired = 4 + (distanceA / b->maximum_move) + (distanceB / b->maximum_move);

				if(distanceA > b->fuel) {
					turnsRequired += 4;//5 turns to refuel;
				}

				if(turnsRequired> b->turns_left) { //do we have enough turns left to do this stuff
					current = current->next;
					continue;
				}

				if(totalFuelInWorld(b) + b->fuel + 10 < (distanceB + distanceA)) { //is the world out of fuel what?
					current = current->next;
					continue;
				}	

				profit = totalProfit;
				bestDeal = current;
				*amt = amount;
				fuelNeeded = distanceA;
			}
		}

		current = current->next;
	} while(current != start);
	// printf("best thing to do would be to buy from %s and sell to %s for $%d profit\n", bestDeal->name, bestBuyer->name, profit);
	if(b->fuel - fuelNeeded < abs(nearestFuel(bestDeal))) {
		//ie need fuel sir
		*refuel = 1; // refuel cuz u cant reach that position
	}

	return bestDeal;
}
