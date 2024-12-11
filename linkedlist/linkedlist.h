//Ignacio Giral ignacio.giral
//Llorenç Salvà llorenc.salva

/***********************************************
*
* @Proposit: LINKEDLIST 1: Para los Poole (.h)
* @Autor/s: Ignacio Giral, Llorenç Salvà (ignacio.giral, llorenc.salva)
* @Data creacio: 21/11/2023
* @Data ultima modificacio: 21/06/2024
*
************************************************/

// Define guard to prevent compilation problems if we add the module more
//  than once in the project.
#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>
#include <time.h>

// Constants to manage the list's error codes.
#define LIST_NO_ERROR 0
#define LIST_ERROR_FULL 1			// Error, the list is full.
#define LIST_ERROR_EMPTY 2			// Error, the list is empty.
#define LIST_ERROR_MALLOC 3			// Error, a malloc failed.
#define LIST_ERROR_END 4			// Error, the POV is at the end.

typedef struct{
    int sockfd;
    char* ip;
    char* port;
    char* worker_type;
    char* fleck_username;
} listElement;


// Data types
typedef listElement* Element;

typedef struct list_t* LinkedList;

// Procedures & Functions

/**************************************************************************** 
 *
 * @Objective: Creates an empty linked list.
 *			   If the list fails to create the phantom node, it will set
 *				the error code to LIST_ERROR_MALLOC.
 *
 * @Parameters: ---
 * @Return: An empty linked list
 *
 ****************************************************************************/
LinkedList LINKEDLIST_create ();


/**************************************************************************** 
 *
 * @Objective: Inserts the specified element in this list before the element
 *			    who is the current point of view. Shifts the point of view
 *				element (if any) and any subsequent elements to the right.
 *			   If the list fails to create the new node to store the element,
 *				it will set	the error code to LIST_ERROR_MALLOC.
 *
 * @Parameters: (in/out) list    = the linked list where to add the new element
 *				(in)     element = the element to add to the list
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST_add (LinkedList list, Element element);


/**************************************************************************** 
 *
 * @Objective: Removes the element currently at the point of view in this 
 *				list. Shifts any subsequent elements to the left.
 *			   This operation will fail if the POV is after the last valid
 *				element of the list. That will also happen for an empty list.
 *				In that situation, this operation will set the error code to
 *				LIST_ERROR_END.
 *
 * @Parameters: (in/out) list = the linked list where to remove the element
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST_remove (LinkedList list);


/**************************************************************************** 
 *
 * @Objective: Returns the element currently at the point of view in this list.
 *			   This operation will fail if the POV is after the last valid
 *				element of the list. That will also happen for an empty list.
 *				In that situation, this operation will set the error code to
 *				LIST_ERROR_END.
 * 
 * @Parameters: (in/out) list = the linked list where to get the element.
 *								in/out because we need to set the error code.
 * @Return: ---
 *
 ****************************************************************************/
Element LINKEDLIST_get (LinkedList list);


/**************************************************************************** 
 *
 * @Objective: Returns true (!0) if this list contains no elements.
 * 
 * @Parameters: (in)     list = the linked list to check
 * @Return: true (!0) if this list contains no elements, false (0) otherwise
 *
 ****************************************************************************/
int 	LINKEDLIST_isEmpty (LinkedList list);


/**************************************************************************** 
 *
 * @Objective: Moves the point of view to the first element in the list.
 * 
 * @Parameters: (in/out) list = the linked list to move the POV.
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST_goToHead (LinkedList list);


/**************************************************************************** 
 *
 * @Objective: Moves the point of view to the next element in the list.
 *				If the POV is after the last element in the list (or when 
 *				the list is empty), this function will set the list's error 
 *				to LIST_ERROR_END. 
 * 
 * @Parameters: (in/out) list = the linked list to move the POV.
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST_next (LinkedList list);


/**************************************************************************** 
 *
 * @Objective: Returns true (!0) if the POV is after the last element in the
 *				list.
 * 
 * @Parameters: (in)     list = the linked to check.
 * @Return: true (!0) if the POV is after the last element in the list
 *
 ****************************************************************************/
int 	LINKEDLIST_isAtEnd (LinkedList list);


//dos nuevas funciones
void LINKEDLIST_clear(LinkedList list);
void LINKEDLIST_shuffle(LinkedList list);




/**************************************************************************** 
 *
 * @Objective: Removes all the elements from the list and frees any dynamic
 *				memory block the list was using. The list must be created
 *				again before usage.
 * 
 * @Parameters: (in/out) list = the linked list to destroy.
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST_destroy (LinkedList* list);


/**************************************************************************** 
 *
 * @Objective: This function returns the error code provided by the last 
 *				operation run. The operations that update the error code are:
 *				Create, Add, Remove and Get.
 * 
 * @Parameters: (in)     list = the linked list to check.
 * @Return: an error code from the list of constants defined.
 *
 ****************************************************************************/
int		LINKEDLIST_getErrorCode (LinkedList list);


#endif
