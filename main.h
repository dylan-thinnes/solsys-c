#include <msieve.h>
#include <primecount.h>
#include <li.h>

#include <signal.h>
#include <string.h>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

typedef struct composite {
    char* value;
    struct factor* factors;
} composite;

typedef struct factor {
    struct composite* spacer;
    struct factor* next;
    char* base;
	enum msieve_factor_type factor_type;
    struct composite* power;
} factor;

// fifo queue as worklist
typedef struct worklist {
    composite* todo;
    struct worklist* prev;
    struct worklist* next;
} worklist;

void print_help();
void debug_log(char* format, ...);

void free_composite(composite*, int freenumber);
void free_factor(factor*);
void print_composite(composite*);
void print_composite_indent(composite*, int depth);

void free_worklist(worklist*);
composite* append (worklist*, char* number);

msieve_obj *g_curr_factorization;
void handle_signal(int sig);
void get_random_seeds(uint32* seed1, uint32* seed2);
msieve_obj * make_default_msieve_obj();
msieve_obj * run_default_msieve();

int simple_demo(char* number);

int recursive_demo(char* number);
composite* factor_composite (char* number);

int is_prefix(char* pre, char* str);