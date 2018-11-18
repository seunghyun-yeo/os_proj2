//	queue ADT Type Defintions 
typedef struct node
{
	void*        dataptr;
	struct node* next;
} queuenode;
typedef struct
{
	queuenode* front;
	queuenode* rear;
	int         count;
} queue;

//	Prototype Declarations 
queue* createqueue(void);
queue* destroyqueue(queue* queue);

bool  dequeue(queue* queue, void** itemptr);
bool  enqueue(queue* queue, void*  itemptr);
bool  queuefront(queue* queue, void** itemptr);
bool  queuerear(queue* queue, void** itemptr);
int   queuecount(queue* queue);
bool  requeue(queue* queue);
bool  movqueue(queue* squeue, queue* dqueue);
bool  emptyqueue(queue* queue);
bool  fullqueue(queue* queue);
void  printqueue(queue* queue);
//	End of queue ADT Definitions 
