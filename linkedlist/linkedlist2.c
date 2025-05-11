
/***********************************************
*
* @Proposito:  Implementa las funciones para la gestión y manipulación de las linkedlists
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#include "linkedlist2.h"
/**************************************************
 *
 * @Estructura: Node
 * @Finalidad: Representar un nodo en la lista enlazada de segundo nivel,
 *             conteniendo un elemento de datos y un puntero al siguiente nodo.
 * @Campos:
 *   - element: Element2 que almacena la información del nodo.
 *   - next:    Puntero al siguiente Node en la secuencia; NULL si es el último.
 *
 **************************************************/
typedef struct _Node {		
	Element2 element;
	struct _Node * next;
} Node;

/**************************************************
 *
 * @Estructura: list_t
 * @Finalidad:  Encapsular el estado de la lista enlazada de segundo nivel,
 *              permitiendo operaciones de recorrido, inserción, eliminación
 *              y gestión de errores.
 * @Campos:
 *   - error:    Código de error de la última operación realizada (0 si no hay fallo).
 *   - head:     Puntero al primer nodo de la lista (head), o nodo fantasma si se usa.
 *   - previous: Puntero al nodo anterior al actual en el recorrido, facilitando
 *               inserciones y eliminaciones en la posición actual.
 *
 **************************************************/
struct list_t {
	int error;
	Node * head;
	Node * previous;
};

/**************************************************
 *
 * @Finalidad: Crear e inicializar una nueva linkedlist,
 *             preparada para almacenar elementos y manejar operaciones
 *             de recorrido, inserción y eliminación.
 * @Parametros: ---
 * @Retorno:    Instancia de LinkedList2 válida y vacía; en caso de error
 *             de asignación de memoria, devuelve NULL.
 *
 **************************************************/
LinkedList2 LINKEDLIST2_create () {
	LinkedList2 list = (LinkedList2) malloc (sizeof(struct list_t));
	
	list->head = (Node*) malloc(sizeof(Node));
	if (NULL != list->head) {
		list->head->next = NULL;
		list->head->element = NULL;  //Evita memoria no inicializada
		list->previous = list->head;
		list->error = LIST_NO_ERROR;
	}
	else {
		list->error = LIST_ERROR_MALLOC;
	}

	return list;
}


/**************************************************
 *
 * @Finalidad: Insertar un nuevo elemento en la linkedlist,
 *             añadiéndolo al final de la secuencia de nodos.
 * @Parametros: in/out: list    = instancia de LinkedList2 a la que se desea agregar
 *                              el nuevo elemento.
 *              in:     element = estructura Element2 que contiene los datos
 *                              del nodo a insertar.
 * @Retorno:    ---
 *
 **************************************************/
void 	LINKEDLIST2_add (LinkedList2 list, Element2 element) {
	if (element == NULL) {
		write(STDERR_FILENO, "[ERROR] Attempt to add a NULL element to the list.\n", 51);
		return;
	}

	Node* new_node = (Node*) malloc (sizeof(Node));
	if (NULL != new_node) {

		new_node->element = element;
		new_node->next = list->previous->next;

		list->previous->next = new_node;
		list->previous = new_node;
		list->error = LIST_NO_ERROR;
	}
	else {
		list->error = LIST_ERROR_MALLOC;
	}
}

/**************************************************
 *
 * @Finalidad: Eliminar el elemento en la posición actual
 *             del cursor de la linkedlist,
 *             liberar la memoria asociada al nodo y ajustar
 *             la lista para que el cursor apunte al siguiente elemento.
 * @Parametros: in/out: list = instancia de LinkedList2 de la cual
 *                           se desea remover el elemento actual.
 * @Retorno:    ---
 *
 **************************************************/
void 	LINKEDLIST2_remove (LinkedList2 list) {
	Node* aux = NULL;

	if (LINKEDLIST2_isAtEnd (list)) {
		list->error = LIST_ERROR_END;
	}
	else {

		aux = list->previous->next;

		list->previous->next = list->previous->next->next;

		free(aux);

		list->error = LIST_NO_ERROR;
	}
}


/**************************************************
 *
 * @Finalidad: Obtener el elemento de datos en la posición
 *             actual del cursor de la linkedlist.
 * @Parametros: in: list = instancia de LinkedList2 cuyo elemento
 *                        actual se desea recuperar.
 * @Retorno:    Estructura Element2 que contiene los datos del nodo
 *             en la posición actual del cursor.
 *
 **************************************************/
Element2 LINKEDLIST2_get (LinkedList2 list) {
	Element2 element;		
	
	if (LINKEDLIST2_isAtEnd (list)) {
		list->error = LIST_ERROR_END;
	}
	else {
		element = list->previous->next->element;
		list->error = LIST_NO_ERROR;
	}

	return element;
}

/**************************************************
 *
 * @Finalidad: Comprobar si la linkedlist está vacía.
 * @Parametros: in: list = instancia de LinkedList2 a verificar.
 * @Retorno:    1 si la lista no contiene elementos;
 *             0 si hay al menos un elemento en la lista.
 *
 **************************************************/
int 	LINKEDLIST2_isEmpty (LinkedList2 list) {
	return NULL == list->head->next;
}


/**************************************************
 *
 * @Finalidad: Mover el cursor de la linkedlist
 *             a la posición inicial (primer elemento) de la lista.
 * @Parametros: in/out: list = instancia de LinkedList2 cuyo cursor
 *                           se desea reiniciar al inicio de la lista.
 * @Retorno:    ---
 *
 **************************************************/
void 	LINKEDLIST2_goToHead (LinkedList2 list) {
	list->previous = list->head;
}


/**************************************************
 *
 * @Finalidad: Avanzar el cursor de la linkedlist
 *             al siguiente elemento disponible.
 * @Parametros: in/out: list = instancia de LinkedList2 cuyo cursor
 *                           se desea mover al siguiente nodo.
 * @Retorno:    ---
 *
 **************************************************/
void 	LINKEDLIST2_next (LinkedList2 list) {
	if (LINKEDLIST2_isAtEnd (list)) {
		list->error = LIST_ERROR_END;
	}
	else {
		list->previous = list->previous->next;

		list->error = LIST_NO_ERROR;
	}
}


/**************************************************
 *
 * @Finalidad: Determinar si el cursor de la linkedlist
 *             ha alcanzado el final de la lista (no hay más elementos disponibles).
 * @Parametros: in: list = instancia de LinkedList2 que se desea verificar.
 * @Retorno:    1 si el elemento actual está en la posición de fin de lista;
 *             0 si aún quedan elementos por recorrer.
 *
 **************************************************/
int 	LINKEDLIST2_isAtEnd (LinkedList2 list) {
	return NULL == list->previous->next;
}

/**************************************************
 *
 * @Finalidad: Eliminar todos los elementos de la linkedlist,
 *             liberando la memoria de sus nodos pero dejando la lista inicializada
 *             y lista para ser reutilizada.
 * @Parametros: in/out: list = instancia de LinkedList2 que se desea limpiar.
 * @Retorno:    ---
 *
 **************************************************/
void LINKEDLIST2_clear(LinkedList2 list) {
    LINKEDLIST2_goToHead(list);
    while (!LINKEDLIST2_isEmpty(list)) {
        LINKEDLIST2_remove(list);
    }
}

/**************************************************
 *
 * @Finalidad: Reorganizar de forma aleatoria los elementos
 *             de la linkedlist, mezclando
 *             el orden original para distribuir las tareas
 *             de manera no determinista.
 * @Parametros: in/out: list = instancia de LinkedList2 cuya
 *                           secuencia de nodos se desea barajar.
 * @Retorno:    ---
 *
 **************************************************/
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
        return;
    }

    LINKEDLIST2_goToHead(list);
    for (int i = 0; i < count; i++) {
        array[i] = LINKEDLIST2_get(list);
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
    LINKEDLIST2_clear(list);  

    for (int i = 0; i < count; i++) {
        LINKEDLIST2_add(list, array[i]);  // Agregar los elementos barajados a la lista
    }

    free(array);
}


/**************************************************
 *
 * @Finalidad: Liberar toda la memoria asociada a la lista
 *             enlazada de segundo nivel, eliminando todos
 *             sus nodos y restableciendo su estado a vacío.
 * @Parametros: in/out: list = puntero a la instancia de LinkedList2
 *                           que se desea destruir; tras la llamada,
 *                           *list quedará invalidado o apuntando a NULL.
 * @Retorno:    ---
 *
 **************************************************/
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

/**************************************************
 *
 * @Finalidad: Obtener el código de error almacenado
 *             en la linkedlist (LinkedList2).
 * @Parametros: in: list = instancia de LinkedList2 de la cual
 *                        se quiere recuperar el código de error.
 * @Retorno:    Entero que indica el último código de error registrado:
 *              0 si no existe error, o un valor positivo que representa
 *              un error específico según la definición interna de LinkedList2.
 *
 **************************************************/
int		LINKEDLIST2_getErrorCode (LinkedList2 list) {
	return list->error;
}
