/*--------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Jason Papadopoulos. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	

$Id: demo.c 984 2015-03-28 16:26:48Z jasonp_sf $
--------------------------------------------------------------------*/

#include "main.h"

int G_DEBUG = 0;
int seed1;
int seed2;

enum demotype { flag_recursive, flag_factorization, flag_logint };

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr, "ERROR: No arguments supplied.\n");
        print_help(*argv);
        exit(1);
    }

    // Detect flags
    enum demotype flag = flag_recursive;
    for (int ii = 1; ii < argc; ii++) {
        if (is_prefix("-r", argv[ii]) || is_prefix("--recursive", argv[ii])) {
            flag = flag_recursive;
            argv[ii] = NULL;
        } else if (is_prefix("-f", argv[ii]) || is_prefix("--factorization", argv[ii])) {
            flag = flag_factorization;
            argv[ii] = NULL;
        } else if (is_prefix("-l", argv[ii]) || is_prefix("--logint", argv[ii])) {
            flag = flag_logint;
            argv[ii] = NULL;
        } else if (is_prefix("-h", argv[ii]) || is_prefix("--help", argv[ii])) {
            print_help(*argv);
            exit(0);
        } else if (is_prefix("-d", argv[ii]) || is_prefix("--debug", argv[ii])) {
            G_DEBUG = 1;
            argv[ii] = NULL;
        }
    }

    // Setup/report demo type
    if (flag == flag_recursive) {
        debug_log("RECURSIVE DEMO\n");
    } else if (flag == flag_logint) {
        debug_log("LOGINT DEMO\n");
        logint_initialize();
    } else {
        debug_log("FACTORIZATION DEMO\n");
    }

    // Set initial seeds
    get_random_seeds(&seed1, &seed2);

    // Run recursive or simple demo on each number
    char* inp = malloc(sizeof(char) * 100);
    for (int ii = 1; ii < argc; ii++) {
        if (argv[ii] == NULL) continue;
        strcpy(inp, argv[ii]);
        if (flag == flag_recursive) {
            recursive_demo(inp);
        } else if (flag == flag_logint) {
            logint_demo(inp);
        } else {
            factorization_demo(inp);
        }
    }
    free(inp);

    // Teardown demo type
    if (flag == flag_logint) {
        logint_free();
    }
}

// Simple checker for string prefixes
int is_prefix (char* pre, char* str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

/*--------------------------------------------------------------------*/
// I/O UTILITIES

void print_help (char* progname) {
    if (progname != NULL) {
        fprintf(stderr, "USAGE: %s <flags> <numbers>\n", progname);
    }
    fprintf(stderr, "FLAGS:\n");
    fprintf(stderr, " -r : run recursive demo <default demo>\n");
    fprintf(stderr, " -f : run factorization demo\n");
    fprintf(stderr, " -l : run logint demo\n");
    fprintf(stderr, " -d : print debug info\n");
    fprintf(stderr, " -h : show help\n");
}

void debug_log (char* format, ...) {
    if (G_DEBUG == 0) return;
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

/*--------------------------------------------------------------------*/
// WORKING WITH COMPOSITES AND FACTORS (FREE AND PRINT)

// Free composites with factors recursively
// Potential stack overflow, but such a deep tree / a cyclical composite-factor
// graph should never exist in the wild
void free_composite (composite* composite, int freenumber) {
    if (composite == NULL) return;
    factor* factor = composite->factors;

    struct factor* next_factor;
    while (factor != NULL) {
        next_factor = factor->next;
        free_factor(factor);
        factor = next_factor;
    }

    // Only free the number if told to - this allows us to not the collect the
    // head of a composite tree, aka the "input"
    if (freenumber) free(composite->value);
    free(composite);
}

// Frees factors with composites recursively
void free_factor (factor* factor) {
    if (factor == NULL) return;
    composite* composite = factor->power;
    free_composite(composite, 1);
    free(factor->base);
    free(factor);
}

// Prints composites recursively
void print_composite (composite* composite) {
    print_composite_indent(composite, 0);
}

void print_composite_indent (composite* composite, int depth) {
    printf("%*c%s\n", depth * 2 + 1, ' ', composite->value);

    factor* factor = composite->factors;
    int index = 0;
    while (factor != NULL) {
        printf("%*c %d:%s\n", depth * 2 + 1, ' ', index, factor->base);
        if (factor->power == NULL) {
        } else {
            print_composite_indent(factor->power, depth+1);
        }
        factor = factor->next;
        index++;
    }
}

/*--------------------------------------------------------------------*/
// WORKING WITH WORKLISTS (FREE AND APPEND)

void free_worklist (worklist* root) {
    if (root == NULL) return;
    worklist* wl = root->next;
    while (wl != root) {
        wl = wl->next;
        free(wl->prev);
    }

    free(root);
}

composite* append (worklist* wl, char* number) {
    worklist* node = malloc(sizeof(worklist));
    node->todo = malloc(sizeof(composite));
    node->todo->value = number;
    node->todo->factors = NULL;

    node->prev = wl;
    node->next = wl->next;
    node->next->prev = node;
    wl->next = node;

    return node->todo;
}

/*--------------------------------------------------------------------*/
// UTILS FOR SETTING UP AN MSIEVE OBJ

msieve_obj *g_curr_factorization = NULL;

void handle_signal(int sig) {

	msieve_obj *obj = g_curr_factorization;

	fprintf(stderr, "\nreceived signal %d; shutting down\n", sig);
	
	if (obj && (obj->flags & MSIEVE_FLAG_SIEVING_IN_PROGRESS))
		obj->flags |= MSIEVE_FLAG_STOP_SIEVING;
	else
		_exit(0);
}

void get_random_seeds(uint32 *seed1, uint32 *seed2) {

	uint32 tmp_seed1, tmp_seed2;

	/* In a multithreaded program, every msieve object
	   should have two unique, non-correlated seeds
	   chosen for it */

#if !defined(WIN32) && !defined(_WIN64)

	FILE *rand_device = fopen("/dev/urandom", "r");

	if (rand_device != NULL) {

		/* Yay! Cryptographic-quality nondeterministic randomness! */

		fread(&tmp_seed1, sizeof(uint32), (size_t)1, rand_device);
		fread(&tmp_seed2, sizeof(uint32), (size_t)1, rand_device);
		fclose(rand_device);
	}
	else

#endif
	{
		/* <Shrug> For everyone else, sample the current time,
		   the high-res timer (hopefully not correlated to the
		   current time), and the process ID. Multithreaded
		   applications should fold in the thread ID too */

		uint64 high_res_time = read_clock();
		tmp_seed1 = ((uint32)(high_res_time >> 32) ^
			     (uint32)time(NULL)) * 
			    (uint32)getpid();
		tmp_seed2 = (uint32)high_res_time;
	}

	/* The final seeds are the result of a multiplicative
	   hash of the initial seeds */

	(*seed1) = tmp_seed1 * ((uint32)40499 * 65543);
	(*seed2) = tmp_seed2 * ((uint32)40499 * 65543);
}

/*--------------------------------------------------------------------*/

msieve_obj * run_default_msieve (char * input) {
    msieve_obj* o = make_default_msieve_obj();

    if (o == NULL) {
        fprintf(stderr, "factoring initialization failed for %s\n", input);
        return NULL;
    }

    o->input = input;
    o->seed1 = seed1;
    o->seed2 = seed2;
    msieve_run(o);
    seed1 = o->seed1;
    seed2 = o->seed2;

    if (!(o->flags & MSIEVE_FLAG_FACTORIZATION_DONE)) {
        fprintf(stderr, "\ncurrent factorization '%s' was interrupted\n", input);
        exit(0);
    }

    return o;
}

msieve_obj * make_default_msieve_obj() {

	char *savefile_name = NULL;
	char *logfile_name = NULL;
	char *infile_name = "worktodo.ini";
	char *nfs_fbfile_name = NULL;
	uint32 flags;
	int32 deadline = 0;
	uint32 max_relations = 0;
	enum cpu_type cpu;
	uint32 cache_size1; 
	uint32 cache_size2; 
	uint32 num_threads = 0;
	uint32 which_gpu = 0;
	const char *nfs_args = NULL;

	flags = MSIEVE_FLAG_USE_LOGFILE;
    flags &= ~(MSIEVE_FLAG_USE_LOGFILE | MSIEVE_FLAG_LOG_TO_STDOUT);

	get_cache_sizes(&cache_size1, &cache_size2);
	cpu = get_cpu_type();

	if (signal(SIGINT, handle_signal) == SIG_ERR) {
	        fprintf(stderr, "could not install handler on SIGINT\n");
	        return NULL;
	}
	if (signal(SIGTERM, handle_signal) == SIG_ERR) {
	        fprintf(stderr, "could not install handler on SIGTERM\n");
	        return NULL;
	}     
#ifdef HAVE_MPI
	{
		int32 level;
		if ((i = MPI_Init_thread(&argc, &argv,
				MPI_THREAD_FUNNELED, &level)) != MPI_SUCCESS) {
			fprintf(stderr, "error %d initializing MPI, aborting\n", i);
			MPI_Abort(MPI_COMM_WORLD, i);
		}
	}
#endif

    msieve_obj* o = msieve_obj_new(NULL, flags,
			    savefile_name, logfile_name,
			    nfs_fbfile_name,
			    0, 0, max_relations,
			    cpu,
			    cache_size1, cache_size2,
			    num_threads, which_gpu, 
			    nfs_args);

    g_curr_factorization = o;
    return o;
}

/*--------------------------------------------------------------------*/

// SIMPLE DEMO, RUNS FACTORIZATION ON EACH ARGV
int factorization_demo(char* number) {

	msieve_factor *factor;

    msieve_obj* o = run_default_msieve(number);
    if (o == NULL) {
        fprintf(stderr, "Demo aborting due to failed factorization.");
        exit(1);
    }

    factor = o->factors;
    while (factor != NULL) {
        char *factor_type;

        if (factor->factor_type == MSIEVE_PRIME)
            factor_type = "p";
        else if (factor->factor_type == MSIEVE_COMPOSITE)
            factor_type = "c";
        else
            factor_type = "prp";

        printf("%s%d %s", factor_type, 
                (int32)strlen(factor->number), 
                factor->number);

        factor = factor->next;
        if (factor != NULL) printf(" ");
    }
    printf("\n");

    msieve_obj_free(o);

#ifdef HAVE_MPI
	MPI_Finalize();
#endif
	return 0;
}

int recursive_demo (char* number) {
    composite* tree = factor_composite(number);
    print_composite(tree);
    free_composite(tree, 0);

    return 0;
}

// TODO: Massive function - refactor and split
composite* factor_composite (char* number) {
	uint32 seed1, seed2;
	get_random_seeds(&seed1, &seed2);

    worklist* head = malloc(sizeof(worklist));
    head->todo = NULL;
    head->prev = head;
    head->next = head;

    composite* full_factor_tree = append(head, number);
    worklist* curr = head->next;

    while (curr->todo != NULL) {
        debug_log("Factoring possible composite: %s\n", curr->todo->value);
        msieve_obj* o = run_default_msieve(curr->todo->value);

        if (o == NULL) {
            fprintf(stderr, "Demo aborting due to failed factorization.");
            exit(1);
        }

        msieve_factor* msieve_factor = o->factors;
        factor* factor_group = NULL;
        int power = 0;
        while (msieve_factor != NULL) {
            if (factor_group != NULL) {
                debug_log("Comparing: %s %s %d\n", msieve_factor->number, factor_group->base, strcmp(factor_group->base, msieve_factor->number));
            }

            if (factor_group != NULL && strcmp(factor_group->base, msieve_factor->number) == 0) {
                power++;
            } else {
                // if the last power group occured more than once, schedule a sub-factorization
                if (factor_group != NULL) {
                    debug_log("Found factors group: %s ^ %d\n", factor_group->base, power);

                    if (power > 1) {
                        char* powstr = malloc(sizeof(char) * 100);
                        sprintf(powstr, "%d", power);
                        composite* composite = append(curr, powstr);
                        factor_group->power = composite;
                    } else {
                        factor_group->power = NULL;
                    }
                }

                // Initialize new factor_group, link to previous if necessary
                if (factor_group != NULL) {
                    factor_group->next = malloc(sizeof(factor));
                    factor_group = factor_group->next;
                } else {
                    factor_group = malloc(sizeof(factor));
                    curr->todo->factors = factor_group;
                }

                // Set next as NULL, copy msieve_factor to base
                factor_group->next = NULL;
                factor_group->factor_type = msieve_factor->factor_type;
                factor_group->base = malloc(sizeof(char) * (strlen(msieve_factor->number) + 1));
                strcpy(factor_group->base, msieve_factor->number);

                power = 1;
            }

            msieve_factor = msieve_factor->next;
        }

        // if the last power group occured more than once, schedule a sub-factorization
        if (factor_group != NULL) {
            debug_log("Found factors group: %s ^ %d\n", factor_group->base, power);

            if (power > 1) {
                char* powstr = malloc(sizeof(char) * 100);
                sprintf(powstr, "%d", power);
                composite* composite = append(curr, powstr);
                factor_group->power = composite;
            } else {
                factor_group->power = NULL;
            }
        }

        msieve_obj_free(o);

        curr = curr->next;
    }

    free_worklist(head);
    return full_factor_tree;
}

// Logarithmic integral demo
int logint_demo (char* number) {
    char* result = logint(number);
    printf("%s\n", result);
    free(result);
    return 0;
}

