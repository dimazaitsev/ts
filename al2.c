// abstract two linlked list basic routines
// gcc -c al2.c

#include <stdio.h>
#include <stdlib.h>

#include "al2.h"

//#define __Al2_MAIN__

void in_l2_tail(struct l2 ** pq, struct l2 * e)
{
  struct l2 *e1, *head, *tail;

  if(*pq==NULL) // empty queue
  {
    *pq=e;
    e->next=e;
    e->prev=e;
  }
  else // non empty queuue
  {
    head=*pq;
    tail=head->prev;
    tail->next=e;
    e->prev=tail;
    e->next=head;
    head->prev=e;
  }
} /* in_l2_head */

void in_l2_order(struct l2 ** pq, struct l2 * e,int (*compare_content)(void *,void *))
{
  struct l2 *curr, *prev, *head;
 
  if(*pq==NULL) // empty queue
  {
    *pq=e;
    e->next=e;
    e->prev=e;
  }
  else
  {
    curr=*pq;
    head=*pq;
    do
    {
      if( (*compare_content)(e->content, curr->content) == -1 )
      {
        prev=curr->prev;
        prev->next=e;
        curr->prev=e;
        e->prev=prev;
        e->next=curr;
        if(curr==head) *pq=e;
        return;
      }
      curr=curr->next;
    }while(curr!=head);
    prev=curr->prev;
    prev->next=e;
    curr->prev=e;
    e->prev=prev;
    e->next=curr;
  }
  
} /* in_l2 */

struct l2 * from_l2_head( struct l2 ** pq )
{
  struct l2 *e, *head, *tail;

  if( *pq == NULL ) return NULL; // no elements

  e=*pq;
  head=e->next;
  tail=e->prev;

  if(e == e->next) // 1 element
  {
    *pq=NULL;
  }
  else // more than 1 elements
  {
    *pq=head;
    head->prev=tail;
    tail->next=head;
  } 
  return e; 
}/* from_l2_head */

struct l2 * from_l2( struct l2 ** pq, void *sample, int (*find_content)(void *,void *))
{
  struct l2 *curr, *head, *prev, *next, *e;

  curr=*pq;
  head=*pq;
  e=NULL;

  if(curr==NULL) return NULL;
  
  do
  {
    if( (*find_content)(sample, curr->content) == 1 )
    {
      e=curr;
      break;
    }
    curr=curr->next;
  }while(curr!=head);
  if(e==NULL) return NULL;

  if(e==e->next)
    *pq=NULL;
  else
  {
    next=e->next;
    prev=e->prev;
    prev->next=next;
    next->prev=prev;
    if(e==head) *pq=e->next;
  }
  return e;
} /* from_l2 */

void print_l2( struct l2 *q, void (*print_content)(void *) )
{
  struct l2 * e = q;
  struct l2 * head = q;

  if(e==NULL)return;
  do
  {
    (*print_content)(e->content);
    e=e->next;
  }while(e!=head);
  printf("\n");
}/* print_l2 */

void print_back_l2( struct l2 *q, void (*print_content)(void *) )
{
  struct l2 *e, *tail;

  if(e==NULL)return;
  
  e = q->prev;
  tail = q->prev;

  do
  {
    (*print_content)(e->content);
    e=e->prev;
  }while(e!=tail);
  printf("\n");
}/* print_back_l2 */


#ifdef __Al2_MAIN__

// an example of using for content of int
// gcc -o al2 al2.c

typedef int tcontent; 

// void (*print_content)(void *c);
// int (*compare_content)(void *c1,void *c2);
// int (*find_content)(void *sample,void *c);

void my_print_content(void *c)
{
  tcontent *p = (tcontent *)c;
  printf("%d ",*p);
}

int my_compare_content(void *c1, void *c2)
{
  tcontent *p1 = (tcontent *)c1, *p2 = (tcontent *)c2;

  if(*p1 < *p2 ) return -1;
  else if(*p1 > *p2 ) return 1;
  else return 0;
}

int my_find_content(void *sample, void *c)
{
  tcontent *p1 = (tcontent *)sample, *p2 = (tcontent *)c;
  if( *p1 == *p2 ) return 1; else return 0;
}

int main()
{
  struct l2 * q = NULL;
  struct l2 * e2;
  int x=1, v;
  tcontent * co;
  
  while(x!=0)
  {
    printf("0-exit; 1-in tail; 2-from head; 3-print queue; 4-in order; 5-from 6-print back\n");
    scanf("%d",&x);
    switch(x)
    {
      case 0: 
        return 0;
      case 1:
        printf("input element:");
        scanf("%d",&v);
        e2=malloc(sizeof(struct l2));
        co=malloc(sizeof(tcontent));
        *co=v;
        e2->content=(void *)co;
        in_l2_tail(&q, e2);
        break;
      case 2:
        e2=from_l2_head( &q );
        if(e2==NULL) 
          printf("queue is empty\n");
        else
        {
          printf("took ");
          my_print_content(e2->content);
          printf("\n");
          free(e2->content);
          free(e2);
        }
        break;
      case 3:
        print_l2( q, my_print_content );
        break;
      case 4:
        printf("input element:");
        scanf("%d",&v);
        e2=malloc(sizeof(struct l2));
        co=malloc(sizeof(tcontent));
        *co=v;
        e2->content=(void *)co;
        in_l2_order(&q, e2, my_compare_content);
        break;
      case 5:
        printf("input value:");
        scanf("%d",&v);
        e2=from_l2( &q, (void *)&v, my_find_content );
        if(e2==NULL) 
          printf("no such element\n");
        else
        {
          printf("took ");
          my_print_content(e2->content);
          printf("\n");
          free(e2->content);
          free(e2);
        }
        break;
      case 6:
        print_back_l2( q, my_print_content );
        break;
    };
  }
} /* main */
#endif

