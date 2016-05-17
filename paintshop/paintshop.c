#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>
#include <queue.h>
#include "paintshop.h"


int staff_go_home, ranTime;
struct semaphore *staff_go_home_access;
struct semaphore *staff_home_lock;
struct semaphore *myMix;
struct semaphore *print;

/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* our algorithm: 
 * 1. customer will put their order in a buffer and wait for their color.
 * 2. staff will take request from buffer and make that color.
 * 3. after mixing staff will put color in a ready buffer.
 * 4. customer will will take color out.
 * 5. If customer has no farther request then he will go home.
 * 6. If no customer left then staff will go home.
*/

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_paint()
 *
 * Takes one argument specifying the can to be filled. The function
 * makes the can available to staff threads and then blocks until the staff
 * have filled the can with the appropriately tinted paint.
 *
 * The can itself contains an array of requested tints.
 */ 

void order_paint(struct paintcan *can)
{
	/* Submit order and put it into a buffer */
	P(order_buffer_access);
	q_addtail(order_buffer, can);
	V(order_buffer_access);
	/* then wait untill customer gets his color back */
	int customer_done = 0;
	while(!customer_done){
		int i;
		P(ready_buffer_access);
		for(i=0;i<NCUSTOMERS && !customer_done;i++){
			if(can == (struct paintcan *)ready_buffer[i]){
				customer_done = 1;
				ready_buffer[i] = NULL;
			}
		}
		V(ready_buffer_access);
		/* if he can't get is color then let other thread to run so that they can produce his color */
		if(!customer_done)thread_yield();
	}
}



/*
 * go_home()
 *
 * This function is called by customers when they go home. It could be
 * used to keep track of the number of remaining customers to allow
 * paint shop staff threads to exit when no customers remain.
 */

void go_home()
{
	/* customer going home so number will be reduced  and no one should interfare during this time */
	//P(customer_number_access);
	customer_number--;
	//V(customer_number_access);
}


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY PAINT SHOP STAFF THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it records the details, and returns a
 * pointer to something representing the order.
 *
 * The return pointer type is void * to allow freedom of representation
 * of orders.
 *
 * The function can return NULL to signal the staff thread it can now
 * exit as their are no customers nor orders left. 
 */
 
void * take_order()
{
	/* staff should wait untill he gets any order */
	int go_home = 0;
	P(order_buffer_access);
	while(q_empty(order_buffer)){
		V(order_buffer_access);
		P(customer_number_access);
		if(customer_number == 0){
			/* if no customer left then staff can go home */
			V(customer_number_access);
			go_home = 1;
			break;
		}
		else{
			/* if there is any customer then let him put his request */
			V(customer_number_access);
			thread_yield();
			P(order_buffer_access);
		}
	}
	if(go_home){
		P(staff_go_home_access);
		staff_go_home++;
		while(staff_go_home < NPAINTSHOPSTAFF){
			V(staff_go_home_access);
			thread_yield();
			P(staff_go_home_access);
		}
		V(staff_go_home_access);
		
		int x = (ranTime++)*10000;
		while(x--);
		return NULL;
	}
	void *order = NULL;
	/* take a request out from the buffer and try to produce that color */
	order = q_remhead(order_buffer);
	V(order_buffer_access);
	return order;
}


/*
 * fill_order()
 *
 * This function takes an order generated by take order and fills the
 * order using the mix() function to tint the paint.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE TINTS
 * IT NEEDS TO USE TO FILE THE ORDER.
 */

void fill_order(void *v)
{
	/* mix color as requested */
	struct paintcan * paint = (struct paintcan *) v;
	int i;
	P(myMix);
	for(i=0;i<PAINT_COMPLEXITY;i++){
		int color_name =  paint->requested_colours[i];
		if(color_name>0)
		P(mixMachine[color_name]);
	}
	V(myMix);
	mix(v);
	//V(myMix);
	P(print);
	for(i=0;i<PAINT_COMPLEXITY;i++){
		int color_name = paint->requested_colours[i];
		if(color_name > 0)
		V(mixMachine[color_name]);
	}
	V(print);
}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to the waiting customer.
 */

void serve_order(void *v)
{
	int i;
	/* while color is ready put it into a ready buffer so that customer can take it from there */
	P(ready_buffer_access);
	for(i=0;i<NCUSTOMERS;i++)if(ready_buffer[i] == NULL){
		ready_buffer[i]=v;
		break;
	}
	V(ready_buffer_access);
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * paintshop_open()
 *
 * Perform any initialisation you need prior to opening the paint shop to
 * staff and customers
 */

void paintshop_open()
{
	customer_number = NCUSTOMERS;
	ranTime = 0;
	customer_number_access = sem_create("customer_number_access",1);
	
	order_buffer = q_create(NCUSTOMERS);
	order_buffer_access = sem_create("order_buffer_access" , 1);
	
	int i;
	for(i=0;i<NCUSTOMERS;i++)ready_buffer[i] = NULL;
	ready_buffer_access = sem_create("ready_buffer_access" , 1);
	staff_go_home = 0;
	staff_go_home_access = sem_create("staff_go_home" , 1);
	//staff_home_lock = sem_create("staff_home_lock" , 0);
	for(i=0;i<NCOLOURS+3;i++)mixMachine[i] = sem_create("mixMachine" , 1);
	myMix = sem_create("myMix" , 1);
	print = sem_create("print" , 1);
}

/*
 * paintshop_close()
 *
 * Perform any cleanup after the paint shop has closed and everybody
 * has gone home.
 */

void paintshop_close()
{
	sem_destroy(customer_number_access);
	q_destroy(order_buffer);
	sem_destroy(order_buffer_access);
	sem_destroy(ready_buffer_access);
	//sem_destroy(staff_home_lock);
	sem_destroy(staff_go_home_access);
	int i;
	for(i=0;i<NCUSTOMERS;i++)ready_buffer[i] = NULL;
	for(i=0;i<NCOLOURS+3;i++)sem_destroy(mixMachine[i]);
	sem_destroy(myMix);
	sem_destroy(print);
}
