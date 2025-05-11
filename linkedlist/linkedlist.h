/***********************************************
*
* @Proposito:  Declara funciones para la gestión y manipulación de las linkedlists
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>
#include <unistd.h>	
#include <time.h>
#include <stdio.h>  
#include <pthread.h>

#define LIST_NO_ERROR 0
#define LIST_ERROR_FULL 1
#define LIST_ERROR_EMPTY 2
#define LIST_ERROR_MALLOC 3
#define LIST_ERROR_END 4

typedef struct{
    int sockfd;
    char* ip;
    char* port;
    char* worker_type;
    char* fleck_username;
    int principal;
    pthread_t thread_id;
} listElement;


// Data types
typedef listElement* Element;
typedef struct list_t* LinkedList;
LinkedList LINKEDLIST_create ();
void 	LINKEDLIST_add (LinkedList list, Element element);
void 	LINKEDLIST_remove (LinkedList list);
Element LINKEDLIST_get (LinkedList list);
int 	LINKEDLIST_isEmpty (LinkedList list);
void 	LINKEDLIST_goToHead (LinkedList list);
void 	LINKEDLIST_next (LinkedList list);
int 	LINKEDLIST_isAtEnd (LinkedList list);


//dos nuevas funciones
void LINKEDLIST_clear(LinkedList list);
void LINKEDLIST_shuffle(LinkedList list);
void 	LINKEDLIST_destroy (LinkedList* list);
int		LINKEDLIST_getErrorCode (LinkedList list);


#endif
