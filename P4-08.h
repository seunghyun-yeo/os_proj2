/*	================== fullqueue =================
This algorithm checks to see if queue is full. It
is full if memory cannot be allocated for next node.
Pre    queue is a pointer to a queue head node
Return true if full; false if room for a node
*/
bool fullqueue(queue* pqueue)
{
	//	Local Definitions *
	queuenode* temp;

	//	Statements 
	temp = (queuenode*)malloc(sizeof(queuenode));
	if (temp)
	{
		free(temp);
		return true;
	} // if 
	  // Heap full 
	return false;
}	// fullqueue 
