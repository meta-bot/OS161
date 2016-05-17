/*
 * **********************************************************************
 * You are free to add anything you think you require to this file
 */
#include "paintshop_driver.h"

struct queue *order_buffer;
struct semaphore *order_buffer_access;

void *ready_buffer[NCUSTOMERS];
struct semaphore *ready_buffer_access;

int customer_number;
struct semaphore *customer_number_access;

struct semaphore *mixMachine[NCOLOURS+3];
