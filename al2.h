// al2.h
// abstract two linked list basic routines

//#ifdef __ABSLIST2__

//#define __ABSLIST2__

struct l2 {
 void * content;
 struct l2 * prev;
 struct l2 * next;
};

// uses user defined procedures doing the element content interpretation
// void (*print_content)(void *c);
// int (*compare_content)(void *c1,void *c2); returns -1,0,1 for <,==,>
// int (*find_content)(void *sample,void *c); returns 1,0 for True,False

void in_l2_tail(struct l2 ** pq, struct l2 * e);
void in_l2_order(struct l2 ** pq, struct l2 * e,int (*compare_content)(void *,void *));
struct l2 * from_l2_head( struct l2 ** pq );
struct l2 * from_l2( struct l2 ** pq, void *sample, int (*find_content)(void *,void *));
void print_l2( struct l2 *q, void (*print_content)(void *) );
void print_back_l2( struct l2 *q, void (*print_content)(void *) );

//#endif

// al2.h end

