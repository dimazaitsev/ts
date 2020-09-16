// gcc -c al2.c
// gcc -o ts ts.c al2.o -lm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h> 

#include "al2.h"

static char help[] =
"Simulator of traffic within d-dimensional torus of size k,\n"
"von Neuman neighborhood, local packet switching rules,\n"
"using shortest paths only (with random load balancing),\n"
"exponential distribution of time between packets,\n"
"node packet queue extraction: the first suitable,\n\n"
"USAGE: ts [options]\n"
"Options (keys):\n"
" --d=dimension,\n"
" --k=size,\n"
" --r=rule: a-f,\n" 
" --cht=channel_time,\n"
" --bl=buffer_length,\n"
" --lambda=node_traffic_intensity (exponential distribution),\n"
" --maxst=halt_simulation_time,\n"
" --dbg=debug_level, = 0,1,2...\n"
"Defaults: ts --d=3 --k=4 --r=a --cht=100 --bl=1000 --maxst=1000000 --dbg=0\n"
"\n";

#define N_OF_NODES(d,k) (pow((k),(d)))
#define N_OF_PORTS(d) (2*(d))
#define N_OF_CHAN(d,k) (N_OF_NODES(d,k)*N_OF_PORTS(d))
#define PORT_DIMENSION(np) ((np) / 2)
#define PORT_DIRECTION(np) (((np)%2==0)?-1:1)
#define TORUS_NEIGHBOR(ij,dij,k) (((ij)+(dij)<0)?((k)-1):((ij)+(dij)>=(k))?0:(ij)+(dij))
#define SIGN(x) (((x)<0)?-1:((x)>0)?1:0)
#define ABS(x) (((x)<0)?-1*(x):(x))

typedef long int simtime;

struct packet {
  int * source;
  int * dest;
  simtime send_time;
  int hops;
  int *da;
};

struct node {
  struct l2 * queue;
  int nq;
  struct l2 ** port_pkt;
};

// events: (a) generate packet np=-1; (b) channel became free np>=0

struct event {
  simtime at;
  int * i;
  int np;
};

// param
int d=3;
int k=4;
int rule='a';
double lambda=0.01;
int cht=100;
int bl=10000;
simtime max_st=1000000;
int dbg=0;

// var
int n_nodes;
int n_ports;
int n_chan;
simtime st=0;
struct l2 * eq = NULL;
struct node *n = NULL;

//stat
long int generated_packets=0;
long int delevered_packets=0;
long int queued_packets=0;
long int dropped_packets=0;
double sum_of_hops=0;
double sum_of_packet_avg_chan_time=0;
double chan_work_time=0;

// abstract list content specific routines ///

int event_compare_content(void * x1, void *x2)
{
  struct event *e1=(struct event *)x1;
  struct event *e2=(struct event *)x2;
  if((e1->at) < (e2->at)) return -1;
  else if((e1->at) > (e2->at)) return 1;
  else return 0;
} /* event_compare_content */

int packet_find_content(void *x1, void *x2)
{
  int * pnp = (int *)x1;
  struct packet *p=(struct packet *)x2;
  int np=*pnp, j;

  j=PORT_DIMENSION(np);
  
  if((p->da[j] != 0) && (SIGN(p->da[j]) == PORT_DIRECTION(np)) ) return 1;
  else return 0; 
} /* packet_find_content */

void event_print_content(void *c)
{
  struct event *e=(struct event *)c;
  int j;
  
  printf("event\n");
  printf("at=%ld, np=%d\n",e->at,e->np);
  for(j=0;j<d;j++) printf("%d ",e->i[j]);
  printf("\n");
  
} /* event_print_content */

/////////////////////////////////////////////

int process_argument(char *a)
{
  if(strncmp(a,"--d=",4)==0) {d=atoi(a+4);return 1;}
  else if(strncmp(a,"--k=",4)==0) {k=atoi(a+4);return 1;}
  else if(strncmp(a,"--r=",4)==0) {rule=a[4];return 1;}
  else if(strncmp(a,"--cht=",6)==0) {cht=atoi(a+6);return 1;}
  else if(strncmp(a,"--bl=",5)==0) {bl=atoi(a+5);return 1;}
  else if(strncmp(a,"--lambda=",9)==0) {lambda=atof(a+9);return 1;}
  else if(strncmp(a,"--maxst=",8)==0) {max_st=atol(a+8);return 1;}
  else if(strncmp(a,"--dbg=",6)==0) {dbg=atoi(a+6);return 1;}
  else if(strncmp(a,"--help",6)==0) {printf("%s",help); return 1;}
  else {printf("%s",help); return 0;}
}

void print_input_info()
{
  printf("***** Input information *****\n");
  printf("torus dimensions d=%d, size k=%d\n",d,k);
  printf("lambda=%le, cht=%d, bl=%d\n",lambda,cht,bl);
  printf("switching rule %c\n\n",rule);
  printf("simulating...\n\n");
}

void print_statistics()
{
  printf("***** Simulation Statistics *****\n");
  printf("simulation time: %ld (mtu)\n",st);
  printf("generated packets: %ld\n",generated_packets);
  printf("delevered packets: %ld\n",delevered_packets);
  printf("queued packets: %ld\n",queued_packets);
  printf("dropped packets: %ld (%le %%)\n",dropped_packets,(double)dropped_packets/delevered_packets*100.0);
  printf("torus performanse: %le (pkt/mtu)\n", ((double)delevered_packets)/st);
  printf("torus load: %le (%%)\n",chan_work_time/(st*n_chan)*100.0 );
  printf("average hops per packet: %le\n",sum_of_hops/delevered_packets);
  printf("average packet channel time: %e (mtu)\n",sum_of_packet_avg_chan_time/delevered_packets);
}

int error_exit(char message[])
{
  fprintf(stderr,"*** error: %s\n",message);
  exit(1);
}

void init_index(int *i, int d)
{
  int j;
  for( j=0; j<d; j++ ) i[j] = 0;
} /* init_index */

int next_index(int *i, int d, int k)
{
   int j=d-1, go = 1, nxt=1;
   
   while( go )
   {
     (i[j])++;
     if( i[j] >= k )
     {
       if( j == 0 ) { go=0; nxt=0; }
       else
       {
	     i[j]=0;
	     j--;
       }
     }
     else go=0;
   } /* while go */
   return nxt;
} /* next_index */

int node_number(int * i, int d, int k)
{
  int j, nn=i[0];
  for(j=1;j<d;j++) { nn*=k; nn+=i[j]; }
  return nn;
} /* node_number */

int port_number(int m, int r)
{
  int np=2*m+((r==-1)?0:1);
  return np;
} /* node_number */

double ran_expo(double lambda)
{
  double u;
  u = rand() / (RAND_MAX + 1.0);
  return -log(1-u) / lambda;
}

simtime packet_interval(double lambda)
{
  simtime dt = ran_expo(lambda);
  if(dt <=0 )dt=1;
  return dt;
} /* packet_interval */

void i_copy(int * from, int * to, int d)
{
  memcpy((void *)to, (void *)from, d*sizeof(int));
} /* i_copy */

void gen_dest( int * source, int * dest, int d, int k )
{
  int j;
  do {
    for(j=0;j<d;j++)dest[j] = rand() % k;
  }while(memcmp((void *)source, (void *)dest, d*sizeof(int))==0);
} /* gen_dest */

int next_hop(int * i,int * ii, int np, int d, int k)
{
  int pd, pr;
  i_copy(i,ii,d);
  pd=PORT_DIMENSION(np);
  pr=PORT_DIRECTION(np);
  ii[pd]=TORUS_NEIGHBOR(i[pd],pr,k);
} /* move_packet_to_netx_hop */

void adr_diff(int *id,int *is, int *di)
{
  int j,da1,da2;
  for(j=0;j<d;j++)
  {
    da1=ABS((id[j]-is[j])%k);
    da2=k-da1;
    if(da1<da2)
    {
      di[j]=da1;
      di[j]*=SIGN((id[j]-is[j])%k);
    }
    else
    {
      di[j]=da2;
      di[j]*=(-1)*SIGN((id[j]-is[j])%k);
    }
  }
} /* adr_diff */

/////////////////////////// rules of packet switching

int sw_pkt_rule_a(struct packet *p, int * i, int nn) // rule a
{
  int j, np;
  
  for(j=0;j<d;j++)
  {
    if( p->da[j]!=0 ) 
    {
      np=port_number(j,SIGN(p->da[j]));

if(dbg>1)
{
printf("packet switched to j=%d, np=%d\n",j,np);
} 

      if( n[nn].port_pkt[np]==NULL)
        return np;
      else return -1;
    }
  }
  return -1;
} /* sw_pkt_rule_a */

int sw_pkt_rule_b(struct packet *p, int * i, int nn) // rule b
{
  int j, np, altp=0,pseqn;
    
  // alternative ports
  for(j=0;j<d;j++)
  {
    if( p->da[j]!=0 )
    {
      altp++;
    } 
  }
  if(altp==0) error_exit("rule b: switching at destination");
  
  // random choice of alternative port
  pseqn=rand()%altp;
  for(j=0;j<d;j++)
  {
    if(p->da[j]!=0)
    {
      if(pseqn<=0) 
      {
         np=port_number(j,SIGN(p->da[j]));
         if( n[nn].port_pkt[np]==NULL)
           return np;
         else return -1;
      }
      pseqn--;
    }
  }
  error_exit("rule b: not switched");
} /* sw_pkt_rule_b */

int sw_pkt_rule_c(struct packet *p, int * i, int nn) // rule c
{
  int j, np, altp=0,rz,z=0;
    
  // alternative ports
  for(j=0;j<d;j++)
  {
    if( p->da[j]!=0 )
    {
      altp++;
      z+=ABS(p->da[j]);
    } 
  }
  if(altp==0) error_exit("rule c: switching at destination");

if(dbg>1)
{
printf("rule c: altp=%d, z=%d\n",altp,z);
} 
  
  // random choice of alternative port
  rz=rand()%z;
  for(j=0;j<d;j++)
  {
    if(p->da[j]!=0)
    {

if(dbg>2)
{
printf("rule c: best available port: da[j]=%d, j=%d, rz=%d\n",p->da[j],j,rz);
}  

      if(rz<=ABS(p->da[j])) 
      {
         np=port_number(j,SIGN(p->da[j]));
         if( n[nn].port_pkt[np]==NULL)
           return np;
         else return -1;
      }
      rz-=ABS(p->da[j]);
    }
  }
  error_exit("rule c: not switched");
} /* sw_pkt_rule_c */

int sw_pkt_rule_d(struct packet *p, int * i, int nn) // rule d
{
  int j, np;
  
  for(j=0;j<d;j++)
  {
    if( p->da[j]!=0 )
    {
      np=port_number(j,SIGN(p->da[j]));

if(dbg>1)
{
printf("packet switched to j=%d, np=%d\n",j,np);
} 

      if( n[nn].port_pkt[np]==NULL)
        return np;
    }
  }
  return -1;
} /* sw_pkt_rule_d */

int sw_pkt_rule_e(struct packet *p, int * i, int nn) // rule e
{
  int j, np, altp=0,pseqn;
  int *dap;

  dap = malloc(d*sizeof(int));
  if( dap==NULL ) error_exit("no memory for addr");
  i_copy(p->da,dap,d);
  
  // availability of alternative ports
  for(j=0;j<d;j++)
  {
    if( p->da[j]!=0 )
    {
      np=port_number(j,SIGN(p->da[j]));
      if( n[nn].port_pkt[np]==NULL)
      {
        altp++;
        
      }
      else
        dap[j]=0;
    } 
  }
  if(altp==0) return -1;
  
  // random choice of alternative port
  pseqn=rand()%altp;
  for(j=0;j<d;j++)
  {
    if(dap[j]!=0)
    {
      if(pseqn==0) break;
      pseqn--;
    }
  }
  np=port_number(j,SIGN(dap[j]));
  return np;
} /* sw_pkt_rule_e */

int sw_pkt_rule_f(struct packet *p, int * i, int nn) // rule e
{
  int j, np, altp=0, rz, z=0;
  int *dap;

  dap = malloc(d*sizeof(int));
  if( dap==NULL ) error_exit("no memory for addr");
  i_copy(p->da,dap,d);
  
  // availability of alternative ports
  for(j=0;j<d;j++)
  {
    if( p->da[j]!=0 )
    {
      np=port_number(j,SIGN(p->da[j]));
      if( n[nn].port_pkt[np]==NULL)
      {
        altp++;
        z+=ABS(p->da[j]);
      }
      else
        dap[j]=0;
    } 
  }
  if(altp==0) return -1;
  
  // random choice of alternative port
  rz=rand()%z;
  for(j=0;j<d;j++)
  {
    if(dap[j]!=0)
    {
      if(rz<=ABS(dap[j])) 
      {
         np=port_number(j,SIGN(dap[j]));
         if( n[nn].port_pkt[np]==NULL)
           return np;
         else return -1;
      }
      rz-=ABS(dap[j]);
    }
  }
  np=port_number(j,SIGN(dap[j]));
  return np;
} /* sw_pkt_rule_f */

//////////////////////////////// END rules of packet switching

int sw_pkt(struct packet *p, int * i) // rule a
{
  int j, nn, np;
  
  adr_diff(p->dest,i,p->da);
  nn = node_number(i,d,k);

if(dbg>1)
{
printf("address difference (");
for(j=0;j<d-1;j++) printf("%d,",p->da[j]);
printf("%d) node %d\n",p->da[d-1],nn);
}

  switch(rule)
  {
  case 'a': np=sw_pkt_rule_a(p,i,nn); break;
  case 'b': np=sw_pkt_rule_b(p,i,nn); break;
  case 'c': np=sw_pkt_rule_c(p,i,nn); break;
  case 'd': np=sw_pkt_rule_d(p,i,nn); break;
  case 'e': np=sw_pkt_rule_e(p,i,nn); break;
  case 'f': np=sw_pkt_rule_f(p,i,nn); break;
  default: error_exit("unknown switching rule");
  }
  return np;
} /* sw_pkt */

void in_pkt(struct l2 *pl2, int *i)
{
  struct packet *p=(struct packet *)pl2->content;
  struct event *e;
  struct l2 *el2;
  int nn, np, j;

if(dbg>1)
{
printf("packet from (");
for(j=0;j<d-1;j++) printf("%d,",(p->source)[j]);
printf("%d) to (",(p->source)[d-1]);
for(j=0;j<d-1;j++) printf("%d,",(p->dest)[j]);
printf("%d) in %ld mtu entered (",(p->dest)[d-1],st-p->send_time);
for(j=0;j<d-1;j++) printf("%d,",i[j]);
printf("%d)\n",i[d-1]);
}

  if(memcmp(i,p->dest,d*sizeof(int))==0)
  {

if(dbg>0)
{
printf("packet delivered from (");
for(j=0;j<d-1;j++) printf("%d,",(p->source)[j]);
printf("%d) to (",(p->source)[d-1]);
for(j=0;j<d-1;j++) printf("%d,",(p->dest)[j]);
printf("%d) in %ld mtu, %d hops\n",(p->dest)[d-1],st-p->send_time,p->hops);
}
    delevered_packets++;
    sum_of_hops+=p->hops;
    sum_of_packet_avg_chan_time+=((double)(st-p->send_time))/p->hops;
    free(p->source);
    free(p->dest);
    free(p->da);
    free(p);
    free(pl2);
    return;
  }

  nn = node_number(i,d,k);
  np=sw_pkt(p,i);
  (p->hops)++;

if(dbg>1)
{
printf("switched to port %d at node %d\n",np,nn);
}

  if(np<0) // if not switched
  {

if(dbg>1)
{
printf("***packet goes to queue\n");
}
    if(n[nn].nq < bl)
    {
      in_l2_tail(&(n[nn].queue),pl2);
      (n[nn].nq)++;
      queued_packets++;
    }
    else
    {
      dropped_packets++;
      free(p->source);
      free(p->dest);
      free(p->da);
      free(p);
      free(pl2);
    }
  }
  else
  {
    // start transmitting packet in port np
    n[nn].port_pkt[np]=pl2;

if(dbg>1)
{
printf("***packet goes to channel\n");
}

    // add packet finish transmitting event
    e = malloc(sizeof(struct event));
    if( e==NULL ) error_exit("no memory for event");
    e->at = st+cht;
    e->np=np;
    e->i = malloc(d*sizeof(int));
    if( e->i==NULL ) error_exit("no memory for index");
    i_copy(i,e->i,d);
    el2 = malloc(sizeof(struct l2));
    if( el2==NULL ) error_exit("no memory for list element");
    el2->content=(void *)e;
    in_l2_order(&eq,el2,event_compare_content);
  }
} /* in_pkt */

int process_event_gen_pkt( struct l2 *el2 )
{
  struct event *e=(struct event *)el2->content;
  struct packet *p;
  struct l2 * pl2;
  int np, nn;
  int *i, *ii;

if(dbg>1)
{
printf("process_event_gen_pkt\n");
}

  // save node address
  i = malloc(d*sizeof(int));
  if( i==NULL ) error_exit("no memory for index");
  i_copy(e->i,i,d);

  // generate a packet
  p = malloc(sizeof(struct packet));
  if( p==NULL ) error_exit("no memory for packet");
  p->send_time=st;
  p->hops=0;

  p->source = malloc(d*sizeof(int));
  if( p->source==NULL ) error_exit("no memory for addr");
  i_copy(e->i,p->source,d);

  p->dest = malloc(d*sizeof(int));
  if( p->dest==NULL ) error_exit("no memory for addr");
  gen_dest( p->source, p->dest, d, k );

  p->da = malloc(d*sizeof(int));
  if( p->da==NULL ) error_exit("no memory for addr");

  pl2 = malloc(sizeof(struct l2));
  if( pl2==NULL ) error_exit("no memory for list element");
  pl2->content=(void *)p;
  generated_packets++;

  // add next packet generation event
  e->at = st + packet_interval(lambda);
  e->np=-1;
  in_l2_order(&eq,el2,event_compare_content);

  in_pkt(pl2,i);
} /* process_event_gen_pkt */

int process_event_free_chan( struct l2 *el2 )
{
  struct event *e=(struct event *)el2->content;
  struct packet *p;
  struct l2 *pl2;
  int np, nn, j;
  int *i, *ii;

if(dbg>1)
{
printf("process_event_free_chan\n");
}
  
  // get node, port numbers and copy address
  nn = node_number(e->i,d,k);
  np = e->np;
  i = malloc(d*sizeof(int));
  if( i==NULL ) error_exit("no memory for index");
  i_copy(e->i,i,d);

  chan_work_time+=cht;

  // move transmitted packet to the next hop
  pl2 = n[nn].port_pkt[np];
if(dbg>1)
{
printf("node=%d, port=%d\n",nn,np);
}
  p=(struct packet *)(pl2->content);

if(dbg>1)
{
printf("packet from (");
for(j=0;j<d-1;j++) printf("%d,",(p->source)[j]);
printf("%d) to (",(p->source)[d-1]);
for(j=0;j<d-1;j++) printf("%d,",(p->dest)[j]);
printf("%d) in %ld mtu freed channel %d in (",(p->dest)[d-1],st-p->send_time,np);
for(j=0;j<d-1;j++) printf("%d,",i[j]);
printf("%d)\n",i[d-1]);
}

  n[nn].port_pkt[np]=NULL;
  ii = malloc(d*sizeof(int));
  if( ii==NULL ) error_exit("no memory for index");
  next_hop(i,ii,np,d,k);
  in_pkt(pl2,ii);
    
  // start next packet transmission on np
  if( (pl2 = from_l2(&(n[nn].queue),&np,packet_find_content) ) != NULL )
  {
    // p=(struct packet *)pl2->content;
if(dbg>0)
{
printf("***packet goes from queue\n");
}
    (n[nn].nq)--;
    queued_packets--;
    n[nn].port_pkt[np]=pl2;
    e->at = st+cht;
    in_l2_order(&eq,el2,event_compare_content);
  }
  else
  {
    free( e );
    free( el2 );
  }  
} /* process_event_free_chan */


int main(int argc, char *argv[])
{
  int *i, nn, j;
  struct event * e;
  struct l2 * el2;

  // process command line arguments
  for(j=1;j<argc;j++)
  {
    if(!process_argument(argv[j])) error_exit("command line error");
  }

  srand((unsigned)time(NULL));

  // allocate and init data
  n_nodes = N_OF_NODES(d,k);
  n_ports = N_OF_PORTS(d);
  n_chan = N_OF_CHAN(d,k);
  print_input_info();

  n = malloc(n_nodes* sizeof(struct node));
  if( n==NULL ) error_exit("no memory for nodes");

  i = malloc(d*sizeof(int));
  if( i==NULL ) error_exit("no memory for index");
 
  init_index(i,d);

  do 
  {
    nn = node_number(i,d,k);
    n[nn].queue = NULL;
    n[nn].nq = 0;
    n[nn].port_pkt = malloc(n_ports*sizeof(struct packet));
    if(n[nn].port_pkt==NULL) error_exit("no memory for ports");
    
    for(j=0;j<n_ports;j++)
    {
      n[nn].port_pkt[j]=NULL;
    }

if(dbg>1)
{
printf("init node (");
for(j=0;j<d-1;j++) printf("%d,",i[j]);
printf("%d)\n",i[d-1]);
}
//getchar();

    // insert a packet generation event
    e = malloc(sizeof(struct event));
    if( e==NULL ) error_exit("no memory for event");

    e->at = packet_interval(lambda);
    e->np=-1;
    e->i = malloc(d*sizeof(int));
    if( e->i==NULL ) error_exit("no memory for index");
    i_copy(i,e->i,d);
    el2 = malloc(sizeof(struct l2));
    if( el2==NULL ) error_exit("no memory for list element");
    el2->content=(void *)e;
    in_l2_order(&eq,el2,event_compare_content);
    
  } while( next_index(i,d,k) );

  // main simulation loop: move time & process current time events

  do
  { 
    // advance simulation time
    if(eq==NULL) error_exit("empty event queue");
    e=(struct event *)eq->content;
    st=e->at;

if(dbg>0)
{
printf("current time: %ld\n",st);
}
//getchar();

    // process all events for simulation time
    while(((struct event *)eq->content)->at <= st)
    {

if(dbg>1)
{
printf("event queue\n");
print_l2(eq,event_print_content);
}
      el2 = from_l2_head( &eq );
      if(((struct event *)el2->content)->np < 0)
        process_event_gen_pkt( el2 );
      else
        process_event_free_chan( el2 );
    }
 
  } while(st <= max_st);

  // print basic statistical info
  print_statistics();

} /* main */

