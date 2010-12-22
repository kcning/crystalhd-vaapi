#ifndef _QUEUE_H_
#define _QUEUE_H_

struct _queue_node {
	struct _queue_node *next;
	void *data;
};
typedef struct _queue_node queue_node;
typedef struct _queue_node * queue_node_p;

struct _queue_base {
	struct _queue_node *front;
	struct _queue_node *rear;
};
typedef struct _queue_base queue_base;
typedef struct _queue_base * queue_base_p;

/* return queue on success, NULL otherwise. */
queue_base_p queue_init( queue_base_p queue );

/* empty a queue */
void queue_destroy( queue_base_p queue );

/* return data on success, NULL otherwise */
queue_node_p queue_enqueue( queue_base_p queue, void * data /* in */ );

/* return 1 on success, 0 if queue is empty */
int queue_dequeue( queue_base_p queue );

/* return the front element on success, NULL if queue is empty */
queue_node_p queue_peek( queue_base_p queue );

/* return 1 if queue is empty, 0 otherwise */
int queue_isempty( queue_base_p queue );

#endif
