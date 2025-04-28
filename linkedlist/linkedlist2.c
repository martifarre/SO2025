//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposit: LINKEDLIST: Para los Workers y Flecks (.c)
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacio: 28/10/2024
* @Data ultima modificacio: 28/10/2024
*
************************************************/

// Libraries
#include <stdlib.h>					// To use dynamic memory.
#include "linkedlist2.h"

/*
 * Node is a recursive structure that will contain each one of the elements.
 * A node has two main fields, the element to store and a pointer to the next
 *  node in the Linear Data Structure.
 * The structure is recursively defined (a Node has a pointer to another node),
 *  so we need to define a new type (typedef) from a structure (struct _Node).
 */
typedef struct _Node {		
	Element2 element;
	struct _Node * next;
} Node;



/*
 * A linked list is a linear data structure, in which the elements are not 
 *  stored at contiguous memory locations. The elements in a linked list 
 *  are stored inside Nodes that are linked using pointers.
 *
 *  +---+----+     +---+----+     +----+----+ 
 *  | 1 |  o-|---> | 2 |  o-|---> | 3  |NULL| 
 *  +---+----+     +---+----+     +----+----+
 *
 * This implementation of the linked list will be using an auxiliary Node
 *  we call the "phantom node". This auxiliary node will help us with the
 *  different operations from the list. It solves the problem of the list
 *  being empty (empty == no nodes) and let us assume that we will always
 *  have one node in the list.
 *
 * Example of an empty list:
 *  
 *               Phantom node
 *       +---+   +---+----+
 *  head | o-|-->|   |NULL|
 *       +---+   +---+----+
 *
 * The linked list will have a "Point of View" (POV). This point of view is the
 *  element (Node) we are visiting at the moment from the list. Whenever 
 *  we decide to add, remove or get an element, we will work from the point 
 *  of view. This point of view is represented by the "previous" pointer in
 *  the LinkedList type. This previous pointer will always point to "the 
 *  element before the point of view". That is why is called previous. We need
 *  to point to the element before the point of view to be able to add new
 *  elements before the first element.
 *
 *        +---+
 *   head | o-|---------
 *        +---+         |
 *   prev | o-|---------|-----------
 *        +---+         |           |
 *                      v           v          Point of View
 *                    +---+---+   +---+---+     +---+---+     +---+----+ 
 *                    |   | o-|-->| 1 | o-|---> | 2 | o-|---> | 3 |NULL| 
 *                    +---+---+   +---+---+     +---+---+     +---+----+
 *
 */
struct list_t {
	int error;			// Error code to keep track of failing operations;
	Node * head;	 	// Head/First element or Phantom node;
	Node * previous; 	// Previous node before the point of view;
};

/**************************************************************************** 
 *
 * @Objective: Creates an empty linked list.
 *			   If the list fails to create the phantom node, it will set
 *				the error code to LIST_ERROR_MALLOC.
 *
 *        +---+
 *   head | o-|---------
 *        +---+         |
 *   prev | o-|---------|--
 *        +---+         | |
 *                      v v          Point of View (After the last element)
 *                    +---+----+   
 *                    |   |NULL| 
 *                    +---+----+   
 *
 * @Parameters: ---
 * @Return: An empty linked list
 *
 ****************************************************************************/
LinkedList2 LINKEDLIST2_create () {
	LinkedList2 list = (LinkedList2) malloc (sizeof(struct list_t));
	
	// Request a Node. This node will be the auxiliary "Phantom" node.
	// The list's head now is the phantom node.
	list->head = (Node*) malloc(sizeof(Node));
	if (NULL != list->head) {
		list->head->next = NULL;
		list->head->element = NULL;  // 🔹 Evita memoria no inicializada
		list->previous = list->head;
		list->error = LIST_NO_ERROR;
	}
	else {
		// Could not get dynamic memory for the phantom node, so we set the
		//  error to malloc error code.
		list->error = LIST_ERROR_MALLOC;
	}

	return list;
}


/**************************************************************************** 
 *
 * @Objective: Inserts the specified element in this list before the element
 *			    who is the current point of view. Shifts the point of view
 *				element (if any) and any subsequent elements to the right.
 *			   If the list fails to create the new node to store the element,
 *				it will set	the error code to LIST_ERROR_MALLOC.
 *
 *        +---+
 *   head | o-|---------
 *        +---+         |
 *   prev | o-|---------|-----------  Will point to 4.
 *        +---+         |           |
 *                      v           v          Point ov View
 *                    +---+---+   +---+---+     +---+---+     +---+----+ 
 *                    |   | o-|-->| 1 | o-|--X->| 2 | o-|---->| 3 |NULL| 
 *                    +---+---+   +---+---+ |   +---+---+     +---+----+
 *										    |         ^
 *										    |         |
 *											|   +---+-|-+
 *										     -->| 4 | o | New Node
 *											    +---+---+
 *
 * @Parameters: (in/out) list    = the linked list where to add the new element
 *				(in)     element = the element to add to the list
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST2_add (LinkedList2 list, Element2 element) {
	if (element == NULL) {
		write(STDERR_FILENO, "[ERROR] Attempt to add a NULL element to the list.\n", 51);
		return;
	}
	// 1- Create a new node to store the new element.
	Node* new_node = (Node*) malloc (sizeof(Node));
	if (NULL != new_node) {
		// 2- Set the element field in the new node with the provided element.
		new_node->element = element;
		// 3- Set the next field in the new node.
		//    The next node will be the node in the point of view.
		new_node->next = list->previous->next;

		// 4- Link the new node to the list. The new node will go before the
		//    point of view, so 
		list->previous->next = new_node;
		// 5- Move the previous pointer.
		list->previous = new_node;

		// As everything was fine, set the error code to NO_ERROR
		list->error = LIST_NO_ERROR;
	}
	else {
		// Could not get dynamic memory for the new node, so we set the
		//  error to malloc error code.
		list->error = LIST_ERROR_MALLOC;
	}
}

/**************************************************************************** 
 *
 * @Objective: Removes the element currently at the point of view in this 
 *				list. Shifts any subsequent elements to the left.
 *			   This operation will fail if the POV is after the last valid
 *				element of the list. That will also happen for an empty list.
 *				In that situation, this operation will set the error code to
 *				LIST_ERROR_END.
  *
 *        +---+
 *   head | o-|---------                        aux (free aux!)
 *        +---+         |                        |
 *   prev | o-|---------|-----------             |
 *        +---+         |           |            |
 *                      v           v          	 v  POV         NEW POV
 *                    +---+---+   +---+---+     +---+---+     +---+----+ 
 *                    |   | o-|-->| 1 | o-|--X->| 2 | o-|---->| 3 |NULL| 
 *                    +---+---+   +---+---+ |   +---+---+     +---+----+
 *										    |                   ^
 *										    |                   |
 *										     -------------------
 *
 * @Parameters: (in/out) list = the linked list where to remove the element
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST2_remove (LinkedList2 list) {
	Node* aux = NULL;
	// We cannot remove an element if the POV is not valid.
	// The POV will not be valid when the previous pointer points to the last
	//  node in the list (there is noone after PREVIOUS).
	if (LINKEDLIST2_isAtEnd (list)) {
		list->error = LIST_ERROR_END;
	}
	else {
		// We need to set an auxiliary pointer to point the element we want
		//  to remove (the POV).
		aux = list->previous->next;

		// "Remove" the POV. The element after the PREVIOUS node will be the
		//  element after the POV.
		list->previous->next = list->previous->next->next;

		// Free the POV. Remove the element.
		free(aux);

		// If there are no errors, set error code to NO_ERROR.
		list->error = LIST_NO_ERROR;
	}
}


/**************************************************************************** 
 *
 * @Objective: Returns the element currently at the point of view in this list.
 *			   If the list is empty, this function will set the list's error 
 *				to LIST_ERROR_EMPTY and the element returned will be undefined.
 * 
 * @Parameters: (in/out) list = the linked list where to get the element
 * @Return: ---
 *
 ****************************************************************************/
Element2 LINKEDLIST2_get (LinkedList2 list) {
	Element2 element;		
	
	// We cannot return an element if the POV is not valid.
	// The POV will not be valid when the previous pointer points to the last
	//  node in the list (there is noone after PREVIOUS).
	if (LINKEDLIST2_isAtEnd (list)) {
		list->error = LIST_ERROR_END;
	}
	else {
		// The element to return is the element stored in the POV.
		element = list->previous->next->element;

		// If there are no errors, set error code to NO_ERROR.
		list->error = LIST_NO_ERROR;
	}

	return element;
}


/**************************************************************************** 
 *
 * @Objective: Returns true (!0) if this list contains no elements.
 * 
 * @Parameters: (in)     list = the linked list to check
 * @Return: true (!0) if this list contains no elements, false (0) otherwise
 *
 ****************************************************************************/
int 	LINKEDLIST2_isEmpty (LinkedList2 list) {
	// The list will be empty if there are no nodes after the phantom node.
	return NULL == list->head->next;
}


/**************************************************************************** 
 *
 * @Objective: Moves the point of view to the first element in the list.
 * 
 * @Parameters: (in/out) list = the linked list to move the POV.
 * @Return: ---
 *
 ****************************************************************************/
void 	LINKEDLIST2_goToHead (LinkedList2 list) {
	// To move the POV to the first element in the list, we need to point
	//  whoever is before the first element. That is the phantom node.
	list->previous = list->head;
}


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
void 	LINKEDLIST2_next (LinkedList2 list) {
	// We cannot move to the next element if the POV is not valid.
	// The POV will not be valid when the previous pointer points to the last
	//  node in the list (there is noone after PREVIOUS).
	if (LINKEDLIST2_isAtEnd (list)) {
		list->error = LIST_ERROR_END;
	}
	else {
		// Move the POV to the next element.
		list->previous = list->previous->next;

		// If there are no errors, set error code to NO_ERROR.
		list->error = LIST_NO_ERROR;
	}
}


/**************************************************************************** 
 *
 * @Objective: Returns true (!0) if the POV is after the last element in the
 *				list.
 * 
 * @Parameters: (in)     list = the linked to check.
 * @Return: true (!0) if the POV is after the last element in the list
 *
 ****************************************************************************/
int 	LINKEDLIST2_isAtEnd (LinkedList2 list) {
	// To check if the list is at the end (POV after the last element) we 
	//  need to check if there is any Node after the previous pointer.
	return NULL == list->previous->next;
}

void LINKEDLIST2_clear(LinkedList2 list) {
    LINKEDLIST2_goToHead(list);
    while (!LINKEDLIST2_isEmpty(list)) {
        LINKEDLIST2_remove(list);  // Suponiendo que tienes una función para eliminar el elemento actual
    }
}

void LINKEDLIST2_shuffle(LinkedList2 list) {
    if (LINKEDLIST2_isEmpty(list)) {
        return;  // Lista vacía, no hay nada que barajar
    }

    // Contar los elementos de la lista
    int count = 0;
    LINKEDLIST2_goToHead(list);
    while (!LINKEDLIST2_isAtEnd(list)) {
        LINKEDLIST2_next(list);
        count++;
    }

    // Copiar elementos a un array
    Element2* array = (Element2*)malloc(count * sizeof(Element2));
    if (array == NULL) {
        // Si hay error al reservar memoria, salir sin modificar la lista
        return;
    }

    LINKEDLIST2_goToHead(list);
    for (int i = 0; i < count; i++) {
        array[i] = LINKEDLIST2_get(list);  // Suponiendo que LINKEDLIST_get devuelve el elemento actual
        LINKEDLIST2_next(list);
    }

    // Barajar el array con el algoritmo Fisher-Yates
    srand(time(NULL));
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Element2 temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }

    // Vaciar la lista original y reconstruir con el nuevo orden
    LINKEDLIST2_clear(list);  // Suponiendo que tienes una función para vaciar la lista

    for (int i = 0; i < count; i++) {
        LINKEDLIST2_add(list, array[i]);  // Agregar los elementos barajados a la lista
    }

    free(array);  // Liberar la memoria del array
}


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
 void LINKEDLIST2_destroy(LinkedList2* list) {
    if (list == NULL || *list == NULL) {
        return;
    }



    Node* aux;
    int node_count = 0;

    while ((*list)->head != NULL) {
        aux = (*list)->head;
        (*list)->head = (*list)->head->next;
        node_count++;


        if (aux->element) {
            if (aux->element->fileName) free(aux->element->fileName);
            if (aux->element->username) free(aux->element->username);
            if (aux->element->worker_type) free(aux->element->worker_type);
            if (aux->element->factor) free(aux->element->factor);
            if (aux->element->MD5SUM) free(aux->element->MD5SUM);
            if (aux->element->distortedMd5) free(aux->element->distortedMd5);
            if (aux->element->directory) free(aux->element->directory);
            free(aux->element);
        } else {
        }

        free(aux);
    }

    (*list)->head = NULL;
    (*list)->previous = NULL;
    free(*list);
    *list = NULL;

    fflush(stdout);
}

/**************************************************************************** 
 *
 * @Objective: This function returns the error code provided by the last 
 *				operation run. The operations that update the error code are:
 *				Create, Add, Remove and Get.
 * @Parameters: (in)     list = the linked list to check.
 * @Return: an error code from the list of constants defined.
 *
 ****************************************************************************/
int		LINKEDLIST2_getErrorCode (LinkedList2 list) {
	return list->error;
}
