/***********************************************
*
* @Proposito:  Declara funciones para la gestión y manipulación de las linkedlists
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#ifndef LINKEDLIST2_H
#define LINKEDLIST2_H

#include <stdlib.h>
#include <unistd.h>	
#include <time.h>
#include <stdio.h>  

#define LIST_NO_ERROR 0
#define LIST_ERROR_FULL 1
#define LIST_ERROR_EMPTY 2
#define LIST_ERROR_MALLOC 3	
#define LIST_ERROR_END 4

typedef struct {
    char *fileName;
    char *username;
    char *worker_type;
    char* factor;
    int bytes_writtenF1;
    int bytes_to_writeF1;
    int bytes_writtenF2;
    int bytes_to_writeF2;
    int fd;
    char* MD5SUM;
    char* distortedMd5;
    char *directory;
    pthread_t thread_id;
    int status; //0: No empezada, 1: Transfiriendo1 , 2: Distorsionando, 3: Transfiriendo2, 4: Completada
} listElement2;


typedef listElement2* Element2;

typedef struct list_t* LinkedList2;


LinkedList2 LINKEDLIST2_create ();
void 	LINKEDLIST2_add (LinkedList2 list, Element2 element);
void 	LINKEDLIST2_remove (LinkedList2 list);
Element2 LINKEDLIST2_get (LinkedList2 list);
int 	LINKEDLIST2_isEmpty (LinkedList2 list);
void 	LINKEDLIST2_goToHead (LinkedList2 list);
void 	LINKEDLIST2_next (LinkedList2 list);
int 	LINKEDLIST2_isAtEnd (LinkedList2 list);

void LINKEDLIST2_clear(LinkedList2 list);
void LINKEDLIST2_shuffle(LinkedList2 list);
void 	LINKEDLIST2_destroy (LinkedList2* list);
int		LINKEDLIST2_getErrorCode (LinkedList2 list);


#endif
