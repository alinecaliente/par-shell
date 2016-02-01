/*
 * list.c - implementation of the integer list functions 
 */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "list.h"
#include <sys/types.h>
#include <signal.h>

list_t* lst_new()
{
   list_t *list, *terminals;
   list = (list_t*) malloc(sizeof(list_t));
   terminals = (list_t*) malloc(sizeof(list_t));
   list->first = NULL;
   terminals->first = NULL;
   return list,terminals;
}


void lst_destroy(list_t *list)
{
	struct lst_iitem *item, *nextitem;

	item = list->first;
	while (item != NULL){
		nextitem = item->next;
		free(item);
		item = nextitem;
	}
	free(list);
}

void insert_new_terminal(list_t *terminals, int pid)
{
	lst_iitem_t *item;

	item = (lst_iitem_t *) malloc (sizeof(lst_iitem_t));
	item->pid = pid;
	item->status = 0;
	item->starttime = 0;
	item->endtime = 0;
	item->next = terminals->first;
	terminals->first = item;
}


void insert_new_process(list_t *list, int pid, time_t starttime)
{
	lst_iitem_t *item;

	item = (lst_iitem_t *) malloc (sizeof(lst_iitem_t));
	item->pid = pid;
	item->status = 0;
	item->starttime = 0;
	item->starttime = starttime;
	item->endtime = 0;
	item->next = list->first;
	list->first = item;
}


void update_terminated_process(list_t *list, int pid, int status, time_t endtime)
{
	lst_iitem_t *item, *nextitem;

	item = list->first;
	while(pid != item->pid){
		nextitem = item->next;
		item = nextitem;
	}
	item->status = status;
	item->endtime = endtime;
}

int search_exec_time(list_t *list, int pid)
{
	lst_iitem_t *item, *nextitem;
	int execTime;

	item = list->first;
	while(pid != item->pid){
		nextitem = item->next;
		item = nextitem;
	}
	execTime = (int)difftime((item->endtime), (item->starttime));
	return execTime;
}

void kill_terminals(list_t *terminals)
{
	lst_iitem_t *item, *nextitem;

	item = terminals->first;
	while (item != NULL){
		unlink("/tmp/par-shell-in-%d",item->pid);
		kill(item->pid, SIGKILL);
		item = item->next;
	}
}


void lst_print(list_t *list)
{
	lst_iitem_t *item;

	printf("\n  PID\t STATUS\tELAPSED(seg)\n");
	item = list->first;
	/* while(1){ */ /* use it only to demonstrate gdb potencial */
	while (item != NULL){
		printf("| %d\t|   %d\t|   %d\t|\n", item->pid, item->status, (int)difftime((item->endtime), (item->starttime)));
		item = item->next;
	}
	printf("-- end of list.\n");
}
