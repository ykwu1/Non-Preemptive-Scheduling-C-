//Yun-Ching Wu

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t mutex;

// swap, reverse, itoa work together to convert int to string
void swap(char *x, char *y)
{
    char temp = *x;
    *x = *y;
    *y = temp;
}

void reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end) {
        swap((str+start), (str+end));
        start++;
        end--;
    }
}

char* itoa(int num, char* str, int base) {
    int i = 0;
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    str[i] = '\0';
    reverse(str, i);
    return str;
}

int sum(int i, int j) {
    int s = 0;
    while(i <= j){
        s += i;
        i++;
    }
	printf("sum is %d\n", s);
	return s;
}

int product(int i, int j) {
    int s = 1;
    while(i <= j){
        s *= i;
        i++;
    }
	printf("power is %d\n", s);
	return s;
}

int power(int i, int j) {
	if(j == 0) return 1;
	int p = 1;
	while(0 < j){
		p *= i;
		j--;
	}
	printf("Power is %d\n", p);
	return p;
}

int fibon(int i, int j) {
	int first = 0;
	int second = 1;
	int p = 0;
	if(j == 0) return 0;
	if(j == 1) return 1;
	j = j-1;
	while(0 < j){
		p = first + second;
		first = second;
		second = p;
		j--;
	}
	printf("Fibonacci is %d\n", p);
	return p;
}

// node that only contains the address of scheduled miniPCB
typedef struct voidNode{
    void * ptr;
    struct voidNode * next;
} voidNode;

//shared queue that contains nodes with addresses of scheduled miniPCBs
typedef struct SharedQ{
	voidNode * head;
	voidNode * tail;
} SharedQ;

// stores data from input. Acts as a node with a next pointer
typedef struct miniPCB{
	int procNum;
	int other;
	int op;
	char opChar[16];
	int param1;
	int param2;
	int retval;
	struct miniPCB * next;
} miniPCB;

//first in, first out queue containing miniPCBs (used to create readyQueue)
typedef struct Queue{
	miniPCB * head;
	miniPCB * tail;
} Queue;

// struct to pass data into the thread parameters
typedef struct thdArg{
	Queue * readyQueue;
	SharedQ * FIFO;
	int schedulerID;
	int f2;
} thdArg;

typedef int (*func)(int,int);

//thdArg constructor
void constructThdArg(thdArg * newThdArg, Queue * readyQueue, SharedQ * FIFO, int schedulerID){
	newThdArg->readyQueue = readyQueue;
	newThdArg->FIFO = FIFO;
	newThdArg->schedulerID = schedulerID;
}

//miniPCB constructor for FCFS
miniPCB * constructFCFSMiniPCB(miniPCB * newMiniPCB, int procNum, int op, int param1, int param2){
	newMiniPCB->procNum = procNum;
	newMiniPCB->op = op;
	newMiniPCB->param1 = param1;
	newMiniPCB->param2 = param2;
	newMiniPCB->retval = -1;
	newMiniPCB->next = NULL;
	return newMiniPCB;
}

//miniPCB constructor for SJF and Priority (other contains either nextCPUBurst or priority)
miniPCB * constructOtherMiniPCB(miniPCB * newMiniPCB, int procNum, int op, int other, int param1, int param2){
	newMiniPCB->procNum = procNum;
	newMiniPCB->op = op;
	newMiniPCB->other = other;
	newMiniPCB->param1 = param1;
	newMiniPCB->param2 = param2;
	newMiniPCB->retval = -1;
	newMiniPCB->next = NULL;
	return newMiniPCB;
}

//sets head to dummyNode
miniPCB * initLinkedlist(miniPCB * head){
	miniPCB * dummyNode = (miniPCB*) malloc(sizeof(miniPCB));
	dummyNode->procNum = -9999;
	dummyNode->next = NULL;
	head = dummyNode;
	return head;
}

//checks if List or Queue is empty (not including dummyNode and not for SharedQ)
bool isEmpty(miniPCB * p) {
	if (p->next == NULL)
		return true;
	else
		return false;
}

// inserts newMiniPCB by desired schdeuler (FCFS, SJF, PRIORITY)
miniPCB * insertList(miniPCB * head, miniPCB * newMiniPCB, int schedulerID){
	miniPCB * p = NULL;
	p = head;
	miniPCB * prev = NULL;
	prev = head;
			
	if(isEmpty(p)) {
		p->next = newMiniPCB;
		return head;
	}
	
// FCFS sort by arrival time (procNum) -----------------------------------------------------------
	if(schedulerID == 0){	
		while(p->next != NULL) {
			p = p->next;

			if (p->procNum >= newMiniPCB->procNum) {
				newMiniPCB->next = prev->next;
				prev->next = newMiniPCB;
				return head;
			}
			prev = prev->next;
		}
		p->next = newMiniPCB;
		return head;
	}
// SJF sort by nextCPUBurst time (other) -----------------------------------------------------------
	if(schedulerID == 1){	
		while(p->next != NULL) {
			p = p->next;

			if (p->other >= newMiniPCB->other) {
				newMiniPCB->next = prev->next;
				prev->next = newMiniPCB;
				return head;
			}
			prev = prev->next;
		}
		p->next = newMiniPCB;
		return head;
	}
// PRIORITY sort by priority time (other). if priority is the same, then check arrvial time (procNum) -----------------------------------------------------------
	if(schedulerID == 2){	
		while(p->next != NULL) {
			p = p->next;

			if (p->other > newMiniPCB->other) {
				newMiniPCB->next = prev->next;
				prev->next = newMiniPCB;
				return head;
			}
// case if priority is the same
			if (p->other == newMiniPCB->other && p->procNum >= newMiniPCB->procNum){
				newMiniPCB->next = prev->next;
				prev->next = newMiniPCB;
				return head;
			}
			prev = prev->next;
		}
		p->next = newMiniPCB;
		return head;
	}

}

// returns deleted head from list
miniPCB * deleteHeadOfList(miniPCB * head){
	miniPCB * p = head;
	if (isEmpty(p)) 
		return NULL;
	
	p = p->next;
	head->next = p->next;
	p->next = NULL;
	return p;
}

// prints List or Queue (not required for project: used for testing)
void printList(miniPCB * head){
	char stringBuilder[20000] = "Head-->";
	miniPCB * temp = head;

	while (temp->next != NULL) {
		printf("temp->procNum: %d\n", temp->procNum);
		temp = temp->next;
	}
	printf("tempLast->procNum: %d\n\n", temp->procNum);
}

//op will be used later to indicate the index of the function pointer array for typedef int(*func)(int,int)
int convertOp(char * opChar){
	int op = 0;
	if (strcmp(opChar, "sum") == 0) op = 0;
	else if (strcmp(opChar, "product") == 0) op = 1;
	else if (strcmp(opChar, "power") == 0) op = 2;
	else if (strcmp(opChar, "fibonacci") == 0) op = 3;
	else perror("op error");
	return op;
}

//parses file and stores in miniPCB
miniPCB * getMiniPCB(int f1, int * numCharRead, int sizeOfFile, int schedulerID){
	int numComma = 0;
	char fileChar[1];
	char fileData[12] = "";
	int procNum, op, param1, param2, other;
	char opChar[12];
	
// read characters from inputfile until end of file	FCFS --------------------------------------------------------
	if(schedulerID == 0){
		while(true){
// case for end of file, need to print last miniPCB and return
			if(*numCharRead == sizeOfFile){
				param2 = atoi(fileData);
				printf("param2: %d\n\n", param2);
				strcpy(fileData, "");
				numComma = 0;
				miniPCB * newMiniPCB = (miniPCB*) malloc(sizeof(miniPCB));
				newMiniPCB = constructFCFSMiniPCB(newMiniPCB, procNum, op, param1, param2);
				return newMiniPCB;
			}
		
			read(f1, fileChar, sizeof(fileChar));
			(*numCharRead)++;
				
// if fileChar is a comma, store data appropiately into miniPCB object			
			if(strcmp(fileChar, ",") == 0 || strcmp(fileChar, "\n") == 0){
				numComma++;
				if(numComma == 1){
					procNum = atoi(fileData);
					strcpy(fileData, "");
					printf("procNum: %d\n", procNum);
					continue;
				}
				if(numComma == 2){
					strcpy(opChar, fileData);
					strcpy(fileData, "");
					printf("opChar: %s\n", &opChar);
					op = convertOp(opChar);
					printf("op: %d\n", op);
					continue;
				}
				if(numComma == 3){
					param1 = atoi(fileData);
					strcpy(fileData, "");
					printf("param1: %d\n", param1);
					continue;
				}
// constructs miniPCB and returns it
				if(numComma == 4){
					param2 = atoi(fileData);
					printf("param2: %d\n\n", param2);
					strcpy(fileData, "");
					numComma = 0;
					miniPCB * newMiniPCB = (miniPCB*) malloc(sizeof(miniPCB));
					newMiniPCB = constructFCFSMiniPCB(newMiniPCB, procNum, op, param1, param2);
					return newMiniPCB;
				}
			}
			else{
				fileChar[1] = '\0';
				strcat(fileData, fileChar);
			}
		}
	} 
// SJF or PRIORITY (required because these input files have a different format than FCFS) ----------------------------------------------------------------------------------------	
	if(schedulerID == 1 || schedulerID == 2){
		while(true){
// case for end of file, need to print last miniPCB and return
			if(*numCharRead == sizeOfFile){
				param2 = atoi(fileData);
				printf("param2: %d\n\n", param2);
				strcpy(fileData, "");
				numComma = 0;
				miniPCB * newMiniPCB = (miniPCB*) malloc(sizeof(miniPCB));
				newMiniPCB = constructOtherMiniPCB(newMiniPCB, procNum, op, other, param1, param2);
				return newMiniPCB;
			}
		
			read(f1, fileChar, sizeof(fileChar));
			(*numCharRead)++;
				
// if fileChar is a comma, store data appropiately into miniPCB object			
			if(strcmp(fileChar, ",") == 0 || strcmp(fileChar, "\n") == 0){
				numComma++;
				if(numComma == 1){
					procNum = atoi(fileData);
					strcpy(fileData, "");
					printf("procNum: %d\n", procNum);
					continue;
				}
				if(numComma == 2){
					other = atoi(fileData);
					strcpy(fileData, "");
					printf("other: %d\n", other);
					continue;
				}
				if(numComma == 3){
					strcpy(opChar, fileData);
					strcpy(fileData, "");
					printf("opChar: %s\n", &opChar);
					op = convertOp(opChar);
					printf("op: %d\n", op);
					continue;
				}
				if(numComma == 4){
					param1 = atoi(fileData);
					strcpy(fileData, "");
					printf("param1: %d\n", param1);
					continue;
				}
// constructs miniPCB and returns it
				if(numComma == 5){
					param2 = atoi(fileData);
					printf("param2: %d\n\n", param2);
					strcpy(fileData, "");
					numComma = 0;
					miniPCB * newMiniPCB = (miniPCB*) malloc(sizeof(miniPCB));
					newMiniPCB = constructOtherMiniPCB(newMiniPCB, procNum, op, other, param1, param2);
					return newMiniPCB;
				}
			}
			else{
				fileChar[1] = '\0';
				strcat(fileData, fileChar);
			}
		}
	}
}

//sets Queue's head and tail to dummyNode;
Queue * initQ(Queue * q){
	miniPCB * dummyNode = (miniPCB*) malloc(sizeof(miniPCB));
	dummyNode->procNum = -9999;
	dummyNode->next = NULL;
	q->head = dummyNode;
	q->tail = dummyNode;
	return q;
}

//inserts miniPCB to the tail of Queue
void insertQ(Queue * q, miniPCB * m){
	if (isEmpty(q->head)) {
		q->head->next = m;
		q->tail = m;
	}
	else {
		q->tail->next = m;
		q->tail = m;
	}
}

//returns head of queue, then deletes it from the queue
miniPCB * deleteQHead(Queue * readyQueue){
	miniPCB * p = readyQueue->head;
	if (isEmpty(p)) 
		return NULL;
	
	p = p->next;
	readyQueue->head->next = p->next;
	p->next = NULL;
	return p;
}

// gets next miniPCB in Queue and returns it for all schedulers
miniPCB * FCFS_Scheduler(Queue * q){
	miniPCB * newMiniPCB = deleteQHead(q);
	return newMiniPCB;
}

miniPCB * SJF_Scheduler(Queue * q){
	miniPCB * newMiniPCB = deleteQHead(q);
	return newMiniPCB;
}

miniPCB * PRIORITY_Scheduler(Queue * q){
	miniPCB * newMiniPCB = deleteQHead(q);
	return newMiniPCB;
}

//checks if SharedQ is empty (not including dummyNode)
bool isSharedQEmpty(voidNode * p) {
	if (p->next == NULL)
		return true;
	else
		return false;
}

//sends address of scheduled miniPCB into the shared queue between the dispatcher and logger
void send(voidNode * newVoid, SharedQ * FIFO){
	if (isSharedQEmpty(FIFO->head)) {
		FIFO->head->next = newVoid;
		FIFO->tail = newVoid;
	}
	else {
		FIFO->tail->next = newVoid;
		FIFO->tail = newVoid;
	}
}

//dispatcher thread: computes scheduled miniPCB and returns a new miniPCB with retval. sends() that miniPCB to FIFO. Exits when readyQueue is empty
void * dispatcher_Scheduler(void * thdparam){
	thdArg * thdArg1 = (thdArg*) malloc(sizeof(thdArg));
    thdArg1 = (thdArg*) thdparam;
    miniPCB * qHead = thdArg1->readyQueue->head;
	
	func func_ptr[4] = {sum, product, power, fibon};
	miniPCB * (*scheduler[3])(Queue * q) = {FCFS_Scheduler, SJF_Scheduler, PRIORITY_Scheduler};
    
    while(qHead->next != NULL){
    	miniPCB * newMiniPCB = (*scheduler[thdArg1->schedulerID])(thdArg1->readyQueue);
    	newMiniPCB->retval = (func_ptr[newMiniPCB->op])(newMiniPCB->param1, newMiniPCB->param2);
    	voidNode * newVoid = (voidNode*) malloc(sizeof(voidNode));
    	newVoid->ptr = (void*) newMiniPCB;
    	pthread_mutex_lock(&mutex);
    	send(newVoid, thdArg1->FIFO);
    	pthread_mutex_unlock(&mutex);
    }
    
//send() the dummyNode in readyQueue to tail of FIFO before exiting (used as escape for the logger thread)
	miniPCB * dummyNode = qHead;
	voidNode * voidDummy = (voidNode*) malloc(sizeof(voidNode));
    voidDummy->ptr = (void*) dummyNode;
 	pthread_mutex_lock(&mutex);
    send(voidDummy, thdArg1->FIFO);
   	pthread_mutex_unlock(&mutex);
    return NULL;
}

//gets head of FIFO and deletes it from FIFO. Returns deleted address
voidNode * recv(SharedQ * FIFO){
	voidNode * p = FIFO->head;
	if (isSharedQEmpty(p)) 
		return NULL;
	
	p = p->next;
	FIFO->head->next = p->next;
	p->next = NULL;
	return p;
}

// counts length of string (used for writing)
int stringLen(const char * str){
	int i = 0;
	while(str[i] != 0){
		i++;
	}
	return i;
}

// Output file format of each line (for all scheduling input files): Operation Name,input parameter 1,input parameter 2, return value
void printFinal(miniPCB * m, int f2){
	char s[200] = "";
	char num[sizeof(int)];
	if(m->op == 0)strcat(s, "sum");
	else if(m->op == 1) strcat(s, "product");
	else if(m->op == 2) strcat(s, "power");
	else if(m->op == 3) strcat(s, "fibonacci");
	else perror("op issue");
	strcat(s, ",");
	itoa(m->param1, num, 10);
	strcat(s, num);
	strcat(s, ",");
	itoa(m->param2, num, 10);
	strcat(s, num);
	strcat(s, ",");
	itoa(m->retval, num, 10);
	strcat(s, num);
	strcat(s, "\n");
	write(f2, s, stringLen(s));
}

//2nd thread that prints scheduled miniPCBs in the FIFO
void * logger(void * thdparam){
	thdArg * thdArg2 = (thdArg*) malloc(sizeof(thdArg));
    thdArg2 = (thdArg*) thdparam;
// tries to get head of shared queue until dummyNode is received    
    while(true){
    	pthread_mutex_lock(&mutex);
    	voidNode * newVoid = recv(thdArg2->FIFO);
    	if(newVoid == NULL){
    		pthread_mutex_unlock(&mutex);
    		continue;
    	}
    	else{
    		miniPCB * newMiniPCB = (miniPCB*) newVoid->ptr;
   			printf("FIFO->procNum: %d\n", newMiniPCB->procNum); // delete later
  			printf("FIFO->retval: %d\n", newMiniPCB->retval); // delete later
    		if(newMiniPCB->procNum == -9999) break;
   			printFinal(newMiniPCB, thdArg2->f2);
   		}
    	pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, const char *argv[]){
	int f1,f2, schedulerID;
	int numCharRead = 0;
	
	if(argc != 4){
		printf("Wrong number of command line arguments\n");
		exit(1);
	}
	if((f1 = open(argv[2], O_RDONLY, 0)) == -1){
		perror("f1 can't open");
		exit(1);
	}
	if ((f2 = creat(argv[3], 0644)) == -1){
		printf("Can't create %s \n", argv[2]);
		return 3;
	}
	
	miniPCB * head = NULL;
	head = initLinkedlist(head);
	
	int sizeOfFile = lseek(f1, 0, SEEK_END);
	lseek(f1, 0, SEEK_SET);
	
	if(strcmp(argv[1], "FCFS") == 0){
		schedulerID = 0;
	}
	if(strcmp(argv[1], "SJF") == 0){
		schedulerID = 1;
	}
	if(strcmp(argv[1], "PRIORITY") == 0){
		schedulerID = 2;
	}
// reads until end of file and creates sorted linked list of miniPCBs		
	while(numCharRead != sizeOfFile){
		miniPCB * newMiniPCB = NULL;
		newMiniPCB = getMiniPCB(f1, &numCharRead, sizeOfFile, schedulerID);
		head = insertList(head, newMiniPCB, schedulerID);
	}
	printf("Linkedlist after sort: \n");
	printList(head);
// create readyQueue and insert linkedlist onto the queue		
	Queue * readyQueue = (Queue*) malloc(sizeof(Queue));
	readyQueue = initQ(readyQueue);
	miniPCB * deletedMiniPCB;
	while(true){
		deletedMiniPCB = deleteHeadOfList(head);
		if(deletedMiniPCB == NULL) break;
		insertQ(readyQueue, deletedMiniPCB);
	}
		
	printf("Linkedlist after delete: \n");
	printList(head);
		
	printf("Queue after insert: \n");
	printList(readyQueue->head);
		
//initiate FIFO
	SharedQ * FIFO = (SharedQ*) malloc(sizeof(SharedQ));
	voidNode * voidHead = (voidNode*) malloc(sizeof(voidNode));
	voidHead->next = NULL;
	FIFO->head = voidHead;
	FIFO->tail = voidHead;
	
	thdArg * thdArg1 = (thdArg*) malloc(sizeof(thdArg));
	constructThdArg(thdArg1, readyQueue, FIFO, schedulerID);
		
	thdArg * thdArg2 = (thdArg*) malloc(sizeof(thdArg));
	thdArg2->FIFO = FIFO;
	thdArg2->f2 = f2;
		
//thread creation for dispatcher, takes arguments: {readyQueue, (shared) FIFO, schedulerID}
	pthread_t dispatcher_thread_id;
    printf("Before Dispatcher Thread\n");
  	pthread_create(&dispatcher_thread_id, NULL, dispatcher_Scheduler, (void*) thdArg1);
  	 	
//thread creation for logger, takes arguments: {FIFO, schedulerID}
    pthread_t logger_thread_id;
    printf("Before Logger Thread\n");
  	pthread_create(&logger_thread_id, NULL, logger, (void*) thdArg2);
  	 	
  	pthread_join(dispatcher_thread_id, NULL);
    printf("After Dispatcher Thread\n");
    pthread_join(logger_thread_id, NULL);
    printf("After Logger Thread\n");
    free(FIFO);
    free(readyQueue);
	free(head);
	return 0;
}
