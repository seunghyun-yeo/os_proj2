/*	================= dequeue ================
This algorithm deletes a node from the queue.
Pre    queue has been created
Post   Data pointer to queue front returned and
front element deleted and recycled.
Return true if successful; false if underflow
*/
bool dequeue(queue* pqueue, void** itemptr)
{
	//	Local Definitions 
	queuenode* deleteloc;

	//	Statements 
	if (!pqueue->count)
		return false;

	*itemptr = pqueue->front->dataptr;
	deleteloc = pqueue->front;
	if (pqueue->count == 1)
		// Deleting only item in queue 
		pqueue->rear = pqueue->front = NULL;
	else
		pqueue->front = pqueue->front->next;
	(pqueue->count)--;
	free(deleteloc);

	return true;
}	// dequeue 