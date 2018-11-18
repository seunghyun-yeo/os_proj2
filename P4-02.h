/*	================= createqueue ================
Allocates memory for a queue head node from dynamic
memory and returns its address to the caller.
Pre    nothing
Post   head has been allocated and initialized
Return head if successful; null if overflow
*/
queue* createqueue(void)
{
	//	Local Definitions 
	queue* pqueue;

	//	Statements 
	pqueue = (queue*)malloc(sizeof(queue));
	if (pqueue)
	{
		pqueue->front = NULL;
		pqueue->rear = NULL;
		pqueue->count = 0;
	} // if 
	return pqueue;
}	// createqueue