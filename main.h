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
    // Next factor in linked list
    struct factor* next;

    // Values of factor itself
    char* base;
	enum msieve_factor_type factor_type;
    struct composite* power;
    char* pi;

    // Difference between this factor and preceding factor
    struct composite* spacer;
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

int factorization_demo(char* number);
int primecount_demo(char* number);
int logint_demo(char* number);
int logint_err_demo(char* number);
int recursive_demo(char* number);
int msieve_factor_eq_factor_group (factor* factor_group, msieve_factor* msieve_factor);
void schedule_power (worklist* curr, factor* factor_group, int power);
factor* initialize_factor_group (composite* parent, factor* previous_group, msieve_factor* source);
composite* factor_composite (char* number);

int streq(char* a, char* b);
