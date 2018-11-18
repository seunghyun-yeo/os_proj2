/*	================= enqueue ================
This algorithm inserts data into a queue.
Pre    queue has been created
Post   data have been inserted
Return true if successful, false if overflow
*/
bool enqueue(queue* pqueue, void* itemptr)
{
	//	Local Definitions 
	queuenode* newptr;

	//	Statements 
	if (!(newptr =
		(queuenode*)malloc(sizeof(queuenode))))
		return false;

	newptr->dataptr = itemptr;
	newptr->next = NULL;

	if (pqueue->count == 0)
		// Inserting into null queue 
		pqueue->front = newptr;
	else
		pqueue->rear->next = newptr;

	(pqueue->count)++;
	pqueue->rear = newptr;
	return true;
}	// enqueue 
