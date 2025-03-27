#include <stdio.h>
#include <tinyexpr.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

/* LOG */
/*

 * @loop
 * recheck all possible exit states

                                                                                                                                                                                */
typedef struct {
    char* name;
    double value;
} variable;

typedef struct {
    char* name;
    double start;
    double inc;
    double end;
} loopvar;

int isint(double x)
{
    int num;
    char chr;
    char out[50];
    snprintf(out, 50, "%f", x);

    if (sscanf(out, "%d%c", &num, &chr) != 2 || chr != '\n') {
        return 0;
    } else {
        return 1;
    }
    return 0;
}

int loopingcheck(double start, double inc, double end)
{
    if (inc == 0) {
        return 0;
    }
    if (start > end) {
        if (fmod(start - end, inc) != 0) {
            return 0;
        }
        return 1;
    }
    if (start < end) {
        if (fmod(end - start, inc) != 0) {
            return 0;
        }
        return 1;
    }
    return 0;
}

char* sigfigprint(double result, int sigfigs)
{
    char buffer[16];
    char* outputstr = gcvt(result, 6, buffer);
    // Print scientific notation
    if (sigfigs == 0) {
        return outputstr;
    } else {
        snprintf(outputstr, 16, "%.*e", sigfigs - 1, result);
        return outputstr;
    }
}

int var_count(te_variable* X, char* string, int total)
{
    int tally = 0;
    for (int i = 0; i < total; i++) {
        if (strstr(string, X[i].name)) {
            tally++;
        }
    }
    return tally;
}

void printerr(int x)
{
    switch (x) {
    case 2:
        fprintf(stderr, "uqexpr: duplicate variables were detected\n");
        exit(2);
        break;

    case 6:
        fprintf(stderr, "uqexpr: invalid variable(s) were specified\n");
        exit(6);
        break;

    case 9:
        fprintf(stderr,
                "Usage: ./uqexpr [--looping string] [--sigfigures 2..9] "
                "[--initialise string] [filename]\n");
        exit(9);
        break;
    default:
        break;
    }
}

int argument_check(char* input, char* arguments[])
{

    for (int j = 0; j < 3; j++) {
        if (strstr(input, arguments[j])) {
            return j;
        }
    }
    return -1; // returning -1 will send it to default case
}

void valid_use(int argc, char* argv[])
{
    for (int i = 1; i < argc - 1;
            i += 2) { // skip over any strings that would follow the arguments
        if (strstr(argv[i], "--")
                == FALSE) { // if any file before the arguments exit program
            printerr(9);
        }
    }
}

void funcprint (variable *vars, loopvar *loop, int varnum, int loopnum, int sigfigs) {
        if (vars[0].name != NULL) {
                for (int i = 0 ; i < varnum; i++) {
                        fprintf(stdout,"%s = %s\n",vars[i].name,sigfigprint(vars[i].value,sigfigs));
                }
        }
        else {
                fprintf(stdout,"No variables were identified.");
        }

        if (loop[0].name != NULL) {
                for (int i = 0 ; i < loopnum ; i++) { // incomplete
                        fprintf(stdout, "%s = %s (%s,%s,%s)",loop[i].name,sigfigprint(loop[i].start,sigfigs),
															 sigfigprint(loop[i].start,sigfigs),
															 sigfigprint(loop[i].inc,sigfigs),
															 sigfigprint(loop[i].end,sigfigs));
                }
        }
}
/*
//@loop x "expression"
//if ("@loop @31[a-zA-Z] = %64[^\n] ", Var, eqn) == 2
void funcloop (char* Var, char* eqn, te_variable *vars, loopvar *loop, int
varnum, int loopnum, int sigfigs) { for (int i = 0 ; i < loopnum ; i++) { do {
                        te_expr *expr = te_compile(eqn, vars,
var_count(vars,eqn, varnum),0 ); fprintf(stdout, "%s = %s when %s = %s", Var,
sigfigprint(te_eval(expr),sigfigs), Var, sigfigprint(loop[i].start,sigfigs));
                        te_free(expr);
                        loop[i].start += loop[i].inc;
                }
                while(loop[i].start != loop[i].end);
        }
}
*/
int main(int argc, char* argv[]) // include some while clause for EOF e.g
                                 // fgets()
{

    argc--;
    int init_count = 0;
    int loop_count = 0;
    char sigfigs = 0;
    int value_array = 0;
    int loop_array = 0; // still unused
    char* arguments[3] = {"--initialise", "--looping", "--sigfigures"};

    // number of variables to initialise
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], arguments[0]) == 0) {
            init_count++;
        }
        if (strcmp(argv[i], arguments[1]) == 0) {
            loop_count++;
        }
    }
    variable* vars = malloc(init_count
            * sizeof(variable)); // unsure if i need to cast it to variable*
    te_variable* te_vars = calloc(init_count, sizeof(te_variable));

    // for loop variables
    loopvar* loops = calloc(loop_count, sizeof(loopvar)); // NO free()

    //---------------------------------------------------------------------------------------------------------//

    // command line argument stuff
    if (argc > 1) {
        valid_use(argc, argv);
        for (int i = 1; i < argc; i
                += 2) { // find which arguments to run (-1 to exclude filename)

            char* varname = (char*)calloc(32, sizeof(char));
            double val, f1, f2, f3;

            // valid use check e.g blank argument after --initialise, should
            // exit 9, print to stderr
            if (argv[i + 1] == NULL || strstr(argv[i + 1], "--")) {
                printerr(9);
            }

            switch (argument_check(
                    argv[i], arguments)) { // 0 = initialise ; 1 = looping ; 2 =
                                           // sigfigures

            case 0: // initialise variable (declare them in the variable struct
                    // first then map them to a te_variable later
                if (sscanf(argv[i + 1], " %24[a-zA-Z] = %lf", varname, &val)
                        == 2) {
                    vars[value_array].name = strdup(varname);
                    vars[value_array].value = val;
                    value_array++;
                } else {
                    printerr(6);
                }
                break;
                //-----------------------------------------------------------------------------------------------------------------//
            case 1: // looping values

                if (isalpha(argv[i + 1][1] == 0)) {
                    printerr(6);
                    // free values
                }
                // store in array [var,start,inc,end]

                if (sscanf(argv[i + 1], "%24[a-zA-Z],%lf,%lf,%lf", varname, &f1,
                            &f2, &f3)
                        == 4) {
                    if (loopingcheck(f1, f2, f3) == 1) {
                        loops[loop_array].name = strdup(varname);
                        loops[loop_array].start = f1;
                        loops[loop_array].inc = f2;
                        loops[loop_array].end = f3;
                        loop_array++;

                        break;
                    }
                } else {
                    printerr(6);
                    break;
                }
                break;
                //-----------------------------------------------------------------------------------------------------------------//
            case 2: // sig figures
                if (strlen(argv[i + 1]) == 1) {
                    sigfigs = argv[i + 1][1];
                    if (sigfigs == 1 || isdigit(argv[i + 1]) == 0
                            || strlen(argv[i + 1]) != 1) {
                        printerr(9);
                    }
                    break;
                }

            default:
                break;
            }
		}
            //---------------------------------------------------------------------------------------------------------------//
            if (strstr(argv[argc], "--") == 0
                    && strstr(argv[argc - 1], "--") == 0) { // reaches filename
                FILE* file;
                if (strstr(argv[argc], "./")) { // ignore "./" characters
                    char* inputfile = strtok(argv[1], "./");
                    file = fopen(inputfile, "r");
                    if (!file) {
                        fprintf(stderr,
                                "uqexpr: can't open file \"%s\" for reading\n",
                                inputfile);
                        exit(10);
                    } else {
                        char* inputfile = argv[argc];
                        file = fopen(inputfile, "r");
                        if (!file) {
                            fprintf(stderr,
                                    "uqexpr: can't open file \"%s\" for "
                                    "reading\n",
                                    inputfile);
                            exit(10);
                        }
                    }
                    // RUN STUFF ON FILE

                    char* string = NULL;
                    while (!feof(file)) {
                        char* input = (char*)calloc(32, sizeof(char));
                        char* ditto = (char*)calloc(32, sizeof(char));
                        char* varname = (char*)calloc(32, sizeof(char));
                        double val;

                        fgets(string, sizeof(string), file);
                        if (string[0] == '#') {
                            continue;
                        }
                        // check if string is variable declaration
                        if (sscanf(string, "%24[a-zA-Z] = %lf", varname, &val)
                                == 2) {
                            // check if variable already exists
                            int dupe = 0;
                            for (int i = 0; i < value_array; i++) {
                                if (strcmp(vars[i].name, varname)
                                        == 1) { // something wrong here
                                    vars[i].value = val;
                                    dupe = 1;
                                    printf("%s = %s", varname,
                                            sigfigprint(val, sigfigs));
                                    break;
                                }
                                continue;
                            }

                            if (dupe == 0) {
                                // realloc space for new variable
                                value_array++;
                                vars = realloc(vars,
                                        (value_array + 1)
                                                * sizeof(
                                                        variable)); // might be
                                                                    // redundant
                                te_vars = realloc(te_vars,
                                        (value_array + 1)
                                                * sizeof(te_variable));

                                vars[value_array].name = strdup(varname);
                                vars[value_array].value = val;
                                // add new variable to array
                                te_variable var = {.name = strdup(varname),
                                        .address = &(vars[value_array].value),
                                        .type = TE_VARIABLE,
                                        .context = NULL};
                                te_vars[value_array] = var;

                                printf("%s = %s", vars[value_array].name,
                                        sigfigprint(vars[value_array].value,
                                                sigfigs));
                                continue;
                            }
                        } else if (sscanf(input, " %24[a-zA-Z] = %63[^\n]",
                                           varname, ditto)
                                == 2) {
                            if (strstr(ditto, varname)) {
                                for (int i = 0; i < value_array; i++) {
                                    if (strcmp(varname, vars[i].name) == 0) {
                                        te_expr* expr = te_compile(ditto,
                                                te_vars,
                                                var_count(te_vars, ditto,
                                                        value_array),
                                                0);
                                        if (expr) {
                                            vars[i].value = te_eval(expr);
                                            te_variable var = {
                                                    .name = strdup(varname),
                                                    .address = &(vars[i].value),
                                                    .type = TE_VARIABLE,
                                                    .context = NULL};
                                            te_vars[i] = var;
                                            free(input);
                                            free(ditto);
                                            printf("%s = %s\n", varname,
                                                    sigfigprint(vars[i].value,
                                                            sigfigs));
                                            break;
                                        } else {
                                            fprintf(stderr,
                                                    "Invalid command, "
                                                    "expression or assignment "
                                                    "operation detected\n");
                                            te_free(expr);
                                            free(input);
                                            free(ditto);
                                            break;
                                        }
                                    }
                                }
                                continue;
                            }
						}
						else if (sscanf(input, "@print")) {
							funcprint(vars,loops,value_array,loop_array,sigfigs);
						}
                        else {
                            int err;
                            te_expr* expr = te_compile(
                                    string, te_vars, value_array, &err);

                            if (!expr) {
                                fprintf(stderr,
                                        "Invalid command, expression or "
                                        "assignment operation detected\n");
                                continue;
                            } else {
                                double result = te_eval(expr);
                                printf("Result: %s\n",
                                        sigfigprint(result, sigfigs));
                                continue;
                            }
                        }
                    }
                }

                // check for duplicate variables
                for (int i = 0; i < init_count || i < loop_count; i++) {
                    for (int j = i + 1; j < init_count || j < loop_count; j++) {
                        if (vars[i].name == loops[j].name
                                || vars[i].name == vars[j].name
                                || loops[i].name == loops[j].name) {
                            fprintf(stderr,
                                    "uqexpr: duplicate variables were "
                                    "detected\n");
                            exit(2);
                        }
                    }
                }

                // mapping variable to te_variable
                for (int i = 0; i < init_count; i++) {
                    te_variable var = {.name = vars[i].name,
                            .address = &(vars[i].value),
                            .type = TE_VARIABLE,
                            .context = NULL};
                    te_vars[i] = var;
                }
                for (int j = init_count; j < loop_count; j++) {
                    te_variable var = {.name = loops[loop_array].name,
                            .address = &(loops[loop_array].start),
                            .type = TE_VARIABLE,
                            .context = "LOOP"};
                    te_vars[j] = var;
                    loop_array++;
                }
                // Print out initialised variables
                if (vars[0].name != NULL) {
                    fprintf(stdout, "Variables:\n");
                    for (int i = 0; i < value_array + 1;
                            i++) { // UNSURE ABOUT THE +1
                        if (vars[i].name != NULL) {
                            fprintf(stdout, "%s = %s\n", vars[i].name,
                                    sigfigprint(vars[i].value, sigfigs));
                        } else {
                            break;
                        }
                    }
                } else {
                    printf("No variables were identified.\n");
                }

                // Print out loop variables
                if (loops[0].name != NULL) {
                    fprintf(stdout, "Loop variables:\n");
                    for (int i = 0; i < loop_array + 1;
                            i++) { // UNSURE ABOUT THE +1
                        if (loops[i].name != NULL) {
                            fprintf(stdout, "%s = %s (%s,%s,%s)\n",
                                    loops[i].name,
                                    sigfigprint(loops[i].start, sigfigs),
                                    sigfigprint(loops[i].start, sigfigs),
                                    sigfigprint(loops[i].inc, sigfigs),
                                    sigfigprint(loops[i].end, sigfigs));
                        } else {
                            break;
                        }
                    }
                } else {
                    printf("No loop variables were found.\n");
                }
                while (!feof(stdin)) {
                    char* input = (char*)calloc(32, sizeof(char));
                    char* ditto = (char*)calloc(32, sizeof(char));
                    char* varname = (char*)calloc(32, sizeof(char));
                    double val, f1, f2, f3;
                    scanf(" %[^\n]s", input);
                    if (sscanf(input, " %24[a-zA-Z] = %lf", varname, &val)
                            == 2) {
                        // check if variable already exists
                        int dupe = 0;
                        for (int i = 0; i < value_array; i++) {
                            if (strcmp(vars[i].name, varname)
                                    == 1) { // something wrong here
                                vars[i].value = val;
                                dupe = 1;
                                printf("%s = %s\n", varname,
                                        sigfigprint(val, sigfigs));
                                break;
                            }
                            continue;
                        }

                        if (dupe == 0) {
                            // realloc space for new variable
                            vars = realloc(vars,
                                    (value_array + 1)
                                            * sizeof(variable)); // might be
                                                                 // redundant
                            te_vars = realloc(te_vars,
                                    (value_array + 1) * sizeof(te_variable));

                            vars[value_array].name = strdup(varname);
                            vars[value_array].value = val;
                            // add new variable to array
                            te_variable var = {.name = strdup(varname),
                                    .address = &(vars[value_array].value),
                                    .type = TE_VARIABLE,
                                    .context = NULL};
                            te_vars[value_array] = var;

                            printf("%s = %s\n", vars[value_array].name,
                                    sigfigprint(
                                            vars[value_array].value, sigfigs));
                            free(input);
                            free(ditto);
                            value_array++;
                            continue;
                        }

                    } else if (sscanf(input, "@range %24[a-zA-Z],%lf,%lf,%lf",
                                       varname, &f1, &f2, &f3)
                            == 4) {
                        if (loopingcheck(f1, f2, f3)) {
                            loops[loop_array + value_array].name
                                    = strdup(varname);
                            loops[loop_array + value_array].start = f1;
                            loops[loop_array + value_array].inc = f2;
                            loops[loop_array + value_array].end = f3;
                            printf("%s = %s (%s,%s,%s)\n", varname,
                                    sigfigprint(f1, sigfigs),
                                    sigfigprint(f1, sigfigs),
                                    sigfigprint(f2, sigfigs),
                                    sigfigprint(f3, sigfigs));
                            free(input);
                            free(ditto);
                            continue;
                        }
                    } else if (sscanf(input, " %24[a-zA-Z] = %63[^\n]", varname,
                                       ditto)
                            == 2) {
                        if (strstr(ditto, varname)) {
                            for (int i = 0; i < value_array; i++) {
                                if (strcmp(varname, vars[i].name) == 0) {
                                    te_expr* expr = te_compile(ditto, te_vars,
                                            var_count(te_vars, ditto,
                                                    value_array),
                                            0);
                                    if (expr) {
                                        vars[i].value = te_eval(expr);
                                        te_variable var = {
                                                .name = strdup(varname),
                                                .address = &(vars[i].value),
                                                .type = TE_VARIABLE,
                                                .context = NULL};
                                        te_vars[i] = var;
                                        free(input);
                                        free(ditto);
                                        printf("%s = %s\n", varname,
                                                sigfigprint(vars[i].value,
                                                        sigfigs));
                                        break;
                                    } else {
                                        fprintf(stderr,
                                                "Invalid command, expression "
                                                "or assignment operation "
                                                "detected\n");
                                        te_free(expr);
                                        free(input);
                                        free(ditto);
                                        break;
                                    }
                                }
                            }
                            continue;
                        }
						else if (sscanf(input, "@print")) {
							funcprint(vars,loops,value_array,loop_array,sigfigs);
						}
						else {
                            te_expr* expr = te_compile(ditto, te_vars,
                                    var_count(te_vars, ditto, value_array), 0);
                            if (expr) {
                                vars[value_array].name = strdup(varname);
                                vars[value_array].value = te_eval(expr);
                                te_variable var = {.name = varname,
                                        .address = &(vars[value_array].value),
                                        .type = TE_VARIABLE,
                                        .context = NULL};
                                te_vars[value_array] = var;
                                printf("%s = %s\n", vars[value_array].name,
                                        sigfigprint(vars[value_array].value,
                                                sigfigs));
                                value_array++;
                                continue;
                            } else {
                                fprintf(stderr,
                                        "Invalid command, expression or "
                                        "assignment operation detected\n");
                                te_free(expr);
                                free(input);
                                free(ditto);
                                continue;
                            }
                        }
                    }

                    else if (feof(stdin)) {
                        fprintf(stdout, "Thank you for using uqexpr!\n");
                        free(input);
                        free(ditto);
                       continue;
					}
					else if (sscanf(input,"@print")) {
						funcprint(vars,loops,value_array,loop_array,sigfigs);
					}
                   else {
                        int err;
                        te_expr* expr = te_compile(input, te_vars,
                                var_count(te_vars, input, value_array), &err);
                        if (!expr) {
                            fprintf(stderr,
                                    "Invalid command, expression or assignment "
                                    "operation detected\n");
                            fprintf(stderr, "%d\n", err);
                            free(input);
                            free(ditto);
                            continue;
                        } else {
                            fprintf(stdout, "Result = %s\n",
                                    sigfigprint(te_eval(expr), sigfigs));
                            te_free(expr);
                            free(input);
                            free(ditto);
                            continue;
                        }
                    }

                    fprintf(stdout, "Thank you for using uqexpr!\n");
                    free(varname);
                    // free(expression);
                    exit(0);
                }
            }
		}
            //---------------------------------------------------------------------------------------------//
            // Reading filename from arguments
            else if (argc == 1) {
                FILE* file = NULL;
                if (strstr(argv[1], "./")) { // ignore "./" characters
                    char* inputfile = strtok(argv[1], "./");
                    file = fopen(inputfile, "r");
                } else {
                    char* inputfile = argv[1];
                    file = fopen(inputfile, "r");
                }
                // invalid file
                if (file == NULL) {
                    fprintf(stderr,
                            "uqexpr: can't open file \"%s\" for reading\n",
                            argv[1]);
                    exit(10);
                }
                char* string = NULL;
                while (!feof(file)) {
                    char* input = (char*)calloc(32, sizeof(char));
                    char* ditto = (char*)calloc(32, sizeof(char));
                    char* varname = (char*)calloc(32, sizeof(char));
                    double val;

                    fgets(string, sizeof(string), file);
                    if (string[0] == '#') {
                        continue;
                    }
                    // check if string is variable declaration
                    if (sscanf(string, "%24[a-zA-Z] = %lf", varname, &val)
                            == 2) {
                        // check if variable already exists
                        int dupe = 0;
                        for (int i = 0; i < value_array; i++) {
                            if (strcmp(vars[i].name, varname)
                                    == 1) { // something wrong here
                                vars[i].value = val;
                                dupe = 1;
                                printf("%s = %s", varname,
                                        sigfigprint(val, sigfigs));
                                break;
                            }
                            continue;
                        }

                        if (dupe == 0) {
                            // realloc space for new variable
                            value_array++;
                            vars = realloc(vars,
                                    (value_array + 1)
                                            * sizeof(variable)); // might be
                                                                 // redundant
                            te_vars = realloc(te_vars,
                                    (value_array + 1) * sizeof(te_variable));

                            vars[value_array].name = strdup(varname);
                            vars[value_array].value = val;
                            // add new variable to array
                            te_variable var = {.name = strdup(varname),
                                    .address = &(vars[value_array].value),
                                    .type = TE_VARIABLE,
                                    .context = NULL};
                            te_vars[value_array] = var;

                            printf("%s = %s", vars[value_array].name,
                                    sigfigprint(
                                            vars[value_array].value, sigfigs));
                            continue;
                        }
                    } else if (sscanf(input, " %24[a-zA-Z] = %63[^\n]", varname,
                                       ditto)
                            == 2) {
                        if (strstr(ditto, varname)) {
                            for (int i = 0; i < value_array; i++) {
                                if (strcmp(varname, vars[i].name) == 0) {
                                    te_expr* expr = te_compile(ditto, te_vars,
                                            var_count(te_vars, ditto,
                                                    value_array),
                                            0);
                                    if (expr) {
                                        vars[i].value = te_eval(expr);
                                        te_variable var = {
                                                .name = strdup(varname),
                                                .address = &(vars[i].value),
                                                .type = TE_VARIABLE,
                                                .context = NULL};
                                        te_vars[i] = var;
                                        free(input);
                                        free(ditto);
                                        printf("%s = %s\n", varname,
                                                sigfigprint(vars[i].value,
                                                        sigfigs));
                                        break;
                                    } else {
                                        fprintf(stderr,
                                                "Invalid command, expression "
                                                "or assignment operation "
                                                "detected\n");
                                        te_free(expr);
                                        free(input);
                                        free(ditto);
                                        break;
                                    }
                                }
                            }
                            continue;
                        }

                    } else {
                        int err;
                        te_expr* expr = te_compile(
                                string, te_vars, value_array, &err);

                        if (!expr) {
                            fprintf(stderr,
                                    "Invalid command, expression or assignment "
                                    "operation detected\n");
                            continue;
                        } else {
                            double result = te_eval(expr);
                            printf("Result: %s\n",
                                    sigfigprint(result, sigfigs));
                            continue;
                        }
                    }
                }
            }
            if (argc == 0) { // if no arguments run normal calc
                printf("No loop variables were found\n");
                while (!feof(stdin)) {
                    char* input = (char*)calloc(32, sizeof(char));
                    char* ditto = (char*)calloc(32, sizeof(char));
                    char* varname = (char*)calloc(32, sizeof(char));
                    double val, f1, f2, f3;
                    scanf(" %[^\n]s", input);
                    if (sscanf(input, " %24[a-zA-Z] = %lf", varname, &val)
                            == 2) {
                        vars[value_array].name = strdup(varname);
                        vars[value_array].value = val;
                        te_variable var = {.name = varname,
                                .address = &(vars[value_array].value),
                                .type = TE_VARIABLE,
                                .context = NULL};
                        te_vars[value_array] = var;
                        printf("%s = %s\n", vars[value_array].name,
                                sigfigprint(vars[value_array].value, sigfigs));
                        value_array++;
                        free(input);
                        free(ditto);
                        continue;
                    } else if (sscanf(input, "@range %24[a-zA-Z],%lf,%lf,%lf",
                                       varname, &f1, &f2, &f3)
                            == 4) {
                        if (loopingcheck(f1, f2, f3)) {
                            loops[loop_array].name = strdup(varname);
                            loops[loop_array].start = f1;
                            loops[loop_array].inc = f2;
                            loops[loop_array].end = f3;
                            printf("%s = %s (%s,%s,%s)\n", varname,
                                    sigfigprint(f1, sigfigs),
                                    sigfigprint(f1, sigfigs),
                                    sigfigprint(f2, sigfigs),
                                    sigfigprint(f3, sigfigs));
                            free(input);
                            free(ditto);
                            continue;
                        }
                    } else if (sscanf(input, " %24[a-zA-Z] = %24[^\n]", varname,
                                       ditto)
                            == 2) {
                        if (strstr(ditto, varname)) {
                            for (int i = 0; i < value_array; i++) {
                                if (strcmp(varname, vars[i].name) == 0) {
                                    te_expr* expr = te_compile(ditto, te_vars,
                                            var_count(te_vars, ditto,
                                                    value_array),
                                            0);
                                    if (expr) {
                                        vars[i].value = te_eval(expr);
                                        te_variable var = {
                                                .name = strdup(varname),
                                                .address = &(vars[i].value),
                                                .type = TE_VARIABLE,
                                                .context = NULL};
                                        te_vars[i] = var;
                                        free(input);
                                        free(ditto);
                                        printf("%s = %s\n", varname,
                                                sigfigprint(vars[i].value,
                                                        sigfigs));
                                        break;
                                    } else {
                                        fprintf(stderr,
                                                "Invalid command, expression "
                                                "or assignment operation "
                                                "detected\n");
                                        te_free(expr);
                                        free(input);
                                        free(ditto);
                                        break;
                                    }
                                }
                            }
                            continue;
                        } 

						else {
                            te_expr* expr = te_compile(ditto, te_vars,
                                    var_count(te_vars, ditto, value_array), 0);
                            if (expr) {
                                vars[value_array].name = strdup(varname);
                                vars[value_array].value = te_eval(expr);
                                te_variable var = {.name = varname,
                                        .address = &(vars[value_array].value),
                                        .type = TE_VARIABLE,
                                        .context = NULL};
                                te_vars[value_array] = var;
                                printf("%s = %s\n", vars[value_array].name,
                                        sigfigprint(vars[value_array].value,
                                                sigfigs));
                                value_array++;
                            } else {
                                fprintf(stderr,
                                        "Invalid command, expression or "
                                        "assignment operation detected\n");
                                te_free(expr);
                                free(input);
                                free(ditto);
                                break;
                            }
                        }
                    }

                    else if (feof(stdin)) {
                        fprintf(stdout, "Thank you for using uqexpr!\n");
                        free(input);
                        free(ditto);
                        continue;
                    } else {
                        int err;
                        te_expr* expr = te_compile(input, te_vars,
                                var_count(te_vars, input, value_array), &err);
                        if (!expr) {
                            fprintf(stderr,
                                    "Invalid command, expression or assignment "
                                    "operation detected\n");
                            free(input);
                            free(ditto);
                            continue;
                        } else {
                            fprintf(stdout, "Result = %s\n",
                                    sigfigprint(te_eval(expr), sigfigs));
                            te_free(expr);
                            free(input);
                            free(ditto);
                            continue;
                        }
                    }

                    fprintf(stdout, "Thank you for using uqexpr!\n");
                    free(varname);
                    // free(expression);
                    exit(0);
                }
            }

            // te_free()
            // fclose(file);
            return 0;
        }
