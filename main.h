#include <msieve.h>
#include <primecount.h>
#include <li.h>

#include <gmp.h>

#include <signal.h>
#include <string.h>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

typedef struct composite {
    mpz_t value;
    struct factor* factors;
} composite;

typedef struct factor {
    // Next factor in linked list
    struct factor* next;

    // Values of factor itself
    mpz_t base;
	enum msieve_factor_type factor_type;
    struct composite* power;
    mpz_t pi;

    // Difference between this factor and preceding factor
    struct composite* spacer;
} factor;

// fifo queue as worklist
typedef struct worklist {
    composite* output;
    char* todo;
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
composite* append (worklist*, mpz_t number);

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
int msieve_factor_eq_factor_group (factor* factor_group, mpz_t parsed);
void schedule_power (worklist* curr, factor* factor_group, int power);
factor* initialize_factor_group (composite* parent, factor* previous_group, msieve_factor* source, mpz_t parsed);
composite* factor_composite (char* number);

int streq(char* a, char* b);
