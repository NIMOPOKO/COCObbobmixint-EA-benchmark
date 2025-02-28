/**
 * An example of benchmarking random search on a COCO suite. A grid search optimizer is also
 * implemented and can be used instead of random search.
 *
 * Set the global parameter BUDGET_MULTIPLIER to suit your needs.
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <float.h>

#include "coco.h"

//BENCHMARKING_SETTING
#define PROBLEM_CLASS 1 //0:coco, 1:my_class
#define M_PI	3.141592653589793238462643
//EA_SETTINGS
// #define ALGORITHM 0 //0:de,1:ga
// #define ENCODING 0 //0:basic encoding[0,l], 1:new encoding[0,1], 2:basic2 encoding[0-0.5,l+0.49999]
// #define APPROACH 0//if encoding is 0, 0:L, 1:B, if encoding is 1, 0:U-Lf, 1:U-Lm, 2:U-Lb, 3:U-B, if encoding is 2, 0:U2-L, 1:U2-B

//DE
#define DE_N 100
#define DE_CR 0.9
#define DE_F 0.5

//MY_COCO_SETTINGS
#define NUMBER_OF_PROBLEM 9720
#define NUMBER_OF_TARGET 51

typedef struct my_problem{
  char* function_name;
  double *smallest; //[0,1],[0,3],[0,7],[0,15],[0,31]
  double *largest;
  size_t r;
  double *optimal;
  double *best_solution;
  int evaluate_result[NUMBER_OF_TARGET];
  size_t dimension;
  int evaluation_cnt;
  size_t instance; //location of optimal solution
  size_t end_flag;
}MY_PROBLEM;

double target[NUMBER_OF_TARGET];

void free_problem(MY_PROBLEM* problem);
void f1(const double *x, double *y, size_t dimension, double* optimal);
void f8(const double *x, double *y, size_t dimension, double* optimal);
void f15(const double *x, double *y, size_t dimension, double* optimal);
void my_evaluate_func(const double *x, double *y, const char * function_name, size_t dimension, double * optimal);
MY_PROBLEM* init_problem(coco_random_state_t *random_generator);
void my_example_experiment(const char *file_name, coco_random_state_t *random_generator);

//COCO_SETTINGS
int instance_cnt = 0;

/**
 * The maximal budget for evaluations done by an optimization algorithm equals dimension * BUDGET_MULTIPLIER.
 * Increase the budget multiplier value gradually to see how it affects the runtime.
 */
static const unsigned int BUDGET_MULTIPLIER = 10000;

/**
 * The maximal number of independent restarts allowed for an algorithm that restarts itself.
 */
static const long INDEPENDENT_RESTARTS = 1e5;

/**
 * The random seed. Change if needed.
 */
static const uint32_t RANDOM_SEED = 0xdeadbeef;

/**
 * A function type for evaluation functions, where the first argument is the vector to be evaluated and the
 * second argument the vector to which the evaluation result is stored.
 */
typedef void (*evaluate_function_t)(const double *x, double *y);

/**
 * A pointer to the problem to be optimized (needed in order to simplify the interface between the optimization
 * algorithm and the COCO platform).
 */
static coco_problem_t *PROBLEM; 

/**
 * Calls coco_evaluate_function() to evaluate the objective function
 * of the problem at the point x and stores the result in the vector y
 */
static void evaluate_function(const double *x, double *y) {
  coco_evaluate_function(PROBLEM, x, y);
}

/* Structure and functions needed for timing the experiment */
typedef struct {
	size_t number_of_dimensions;
	size_t current_idx;
	char **output;
	size_t previous_dimension;
	size_t cumulative_evaluations;
	time_t start_time;
	time_t overall_start_time;
} timing_data_t;

//COCO prototype
void example_experiment(const char *suite_name,
                        const char *suite_options,
                        const char *observer_name,
                        const char *observer_options,
                        coco_random_state_t *random_generator);

static timing_data_t *timing_data_initialize(coco_suite_t *suite);
static void timing_data_time_problem(timing_data_t *timing_data, coco_problem_t *problem);
static void timing_data_finalize(timing_data_t *timing_data);
char *get_short_function_number(const char *problem_name);

//EA prototype
void ea_group_initialization(double** population,
                                  size_t dimension,
                                  const double *lower_bounds,
                                  const double *upper_bounds,
                                  coco_random_state_t *random_generator);

void ea_group_encoding(double** x,
                       double** tmp,
                       size_t dimension,
                       const double *lower_bounds,
                       const double *upper_bounds);

void round_vec(double *x,
               size_t dimention_size,
               const double *lower_bounds,
               const double *upper_bounds);

void new_round_vec(double *x,
                   size_t dimention_size,
                   const double *lower_bounds,
                   const double *upper_bounds);

void decoding_vec(double *population,
                  size_t dimention_size,
                  const double *lower_bounds,
                  const double *upper_bounds);

void ea_sd_calc(double* sum,
                double* sum2,
                double** tmp,
                size_t dimension,
                FILE *fp);
 
//ALGPRITHM prototype
void de_nopcm(evaluate_function_t evaluate_func,
                    const size_t dimension,
                    const size_t number_of_objectives,
                    const double *lower_bounds,
                    const double *upper_bounds,
                    const size_t max_budget,
                    coco_random_state_t *random_generator,
                    char *titlestr);

void my_de_nopcm(const char* function_name,
                      const size_t dimension,
                      const size_t number_of_objectives,
                      const double *lower_bounds,
                      const double *upper_bounds,
                      const size_t max_budget,
                      coco_random_state_t *random_generator,
                      char *titlestr,
                      MY_PROBLEM *problem);

/**
 * The main method initializes the random number generator and calls the example experiment on the
 * bbob suite.
 */
int main(void) {
  coco_random_state_t *random_generator = coco_random_new(RANDOM_SEED);
  /* Change the log level to "warning" to get less output */
  coco_set_log_level("info");

  if(PROBLEM_CLASS == 0){
    printf("Running the example experiment... (might take time, be patient)\n");
  }
  else if(PROBLEM_CLASS == 1){
    printf("Running the my example experiment... (might take time, be patient)\n");
  }
  fflush(stdout);

  /**
   * Start the actual experiments on a test suite and use a matching logger, for
   * example one of the following:
   *   bbob                 24 unconstrained noiseless single-objective functions
   *   bbob-biobj           55 unconstrained noiseless bi-objective functions
   *   [bbob-biobj-ext       92 unconstrained noiseless bi-objective functions]
   *   [bbob-constrained*   48 constrained noiseless single-objective functions]
   *   bbob-largescale      24 unconstrained noiseless single-objective functions in large dimension
   *   bbob-mixint          24 unconstrained noiseless single-objective functions with mixed-integer variables
   *   bbob-biobj-mixint    92 unconstrained noiseless bi-objective functions with mixed-integer variables
   *
   * Suites with a star are partly implemented but not yet fully supported.
   *
   * Adapt to your need. Note that the experiment is run according
   * to the settings, defined in example_experiment(...) below.
   */
  coco_set_log_level("info");

  /**
   * For more details on how to change the default suite and observer options, see
   * http://numbbo.github.io/coco-doc/C/#suite-parameters and
   * http://numbbo.github.io/coco-doc/C/#observer-parameters. */

  if(PROBLEM_CLASS == 0){
    if(ALGORITHM == 0){//de
      if(ENCODING == 0){//basic encoding
        if(APPROACH == 0){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:L-DE", random_generator);
        }
        else if(APPROACH == 1){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:B-DE", random_generator);
        }
      }
      else if(ENCODING == 1){//new encoding
        if(APPROACH == 0){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:U-Lf-DE", random_generator);
        }
        else if(APPROACH == 1){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:U-Lm-DE", random_generator);
        }
        else if(APPROACH == 2){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:U-Lb-DE", random_generator);
        }
        else if(APPROACH == 3){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:U-B-DE", random_generator);
        }
      }
      else if(ENCODING == 2){//basic2 encoding
        if(APPROACH == 0){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:U2-L-DE", random_generator);
        }
        else if(APPROACH == 1){
          example_experiment("bbob-mixint", "", "bbob-mixint", "result_folder:U2-B-DE", random_generator);
        }
      }
    }
    else if(ALGORITHM == 1){

    }
  }
  else{
    if(ALGORITHM == 0){//de
      if(ENCODING == 0){//basic encoding
        if(APPROACH == 0){
          my_example_experiment("L-DE", random_generator);
        }
        else if(APPROACH == 1){
          my_example_experiment("B-DE", random_generator);
        }
      }
      else if(ENCODING == 1){//new encoding
        if(APPROACH == 0){
          my_example_experiment("U-Lf-DE", random_generator);
        }
        else if(APPROACH == 1){
          my_example_experiment("U-Lm-DE", random_generator);
        }
        else if(APPROACH == 2){
          my_example_experiment("U-Lb-DE", random_generator);
        }
        else if(APPROACH == 3){
          my_example_experiment("U-B-DE", random_generator);
        }
      }
      else if(ENCODING == 2){//basic2 encoding
        if(APPROACH == 0){
          my_example_experiment("U2-L-DE", random_generator);
        }
        else if(APPROACH == 1){
          my_example_experiment("U2-B-DE", random_generator);
        }
      }
    }
    else if(ALGORITHM == 1){

    }
  }

  printf("Done!\n");
  fflush(stdout);

  coco_random_free(random_generator);

  return 0;
}

//EXAMPLE_EXPERIMENT
/**
 * A simple example of benchmarking random search on a given suite with default instances
 * that can serve also as a timing experiment.
 *
 * @param suite_name Name of the suite (e.g. "bbob" or "bbob-biobj").
 * @param suite_options Options of the suite (e.g. "dimensions: 2,3,5,10,20 instance_indices: 1-5").
 * @param observer_name Name of the observer matching with the chosen suite (e.g. "bbob-biobj"
 * when using the "bbob-biobj-ext" suite).
 * @param observer_options Options of the observer (e.g. "result_folder: folder_name")
 * @param random_generator The random number generator.
 */
void example_experiment(const char *suite_name,
                        const char *suite_options,
                        const char *observer_name,
                        const char *observer_options,
                        coco_random_state_t *random_generator) {
  size_t run;
  coco_suite_t *suite;
  coco_observer_t *observer;
  timing_data_t *timing_data;
  /* Initialize the suite and observer. */
  suite = coco_suite(suite_name, "", suite_options);
  observer = coco_observer(observer_name, observer_options);
  /* Initialize timing */
  timing_data = timing_data_initialize(suite);
  /* Iterate over all problems in the suite */
  while ((PROBLEM = coco_suite_get_next_problem(suite, observer)) != NULL) {
    const char *function_name = coco_problem_get_name(PROBLEM);
    char *short_function_name = get_short_function_number(function_name);
    size_t dimension = coco_problem_get_dimension(PROBLEM);

    //filename select
    char titlestr[128] = "./output/";
    char num[30] = "";
    if(ALGORITHM == 0){
      strcat(titlestr,"de/");
    }
    else if(ALGORITHM == 1){
      strcat(titlestr,"ga/");
    }
    strcat(titlestr,short_function_name);
    sprintf(num, "/%ld", dimension);
    strcat(titlestr,num);
    if(ENCODING == 0){
      if(APPROACH == 0){
        strcat(titlestr,"d/L-");
      }
      else{
        strcat(titlestr,"d/B-");
      }
    }
    else if(ENCODING == 1){
      if(APPROACH == 0){
        strcat(titlestr,"d/U-Lf-");
      }
      else if(APPROACH == 1){
        strcat(titlestr,"d/U-Lm-");
      }
      else if(APPROACH == 2){
        strcat(titlestr,"d/U-Lb-");
      }
      else if(APPROACH == 3){
        strcat(titlestr,"d/U-B-");
      }
    }
    else if(ENCODING == 2){
      if(APPROACH == 0){
        strcat(titlestr,"d/U2-L-");
      }
      else{
        strcat(titlestr,"d/U2-B-");
      }
    }
    sprintf(num, "%d", instance_cnt);
    strcat(titlestr,num);
    strcat(titlestr,".txt");
    /* Run the algorithm at least once */
    for (run = 1; run <= 1 + INDEPENDENT_RESTARTS; run++) {
      long evaluations_done = (long) (coco_problem_get_evaluations(PROBLEM) + coco_problem_get_evaluations_constraints(PROBLEM));
      long evaluations_remaining = (long) (dimension * BUDGET_MULTIPLIER) - evaluations_done;

      /* Break the loop if the target was hit or there are no more remaining evaluations */
      if ((coco_problem_final_target_hit(PROBLEM) && coco_problem_get_number_of_constraints(PROBLEM) == 0) || (evaluations_remaining <= 0))
        break;

      /* Call the optimization algorithm for the remaining number of evaluations */
      if(ALGORITHM == 0){
        de_nopcm(evaluate_function,
                        dimension,
                        coco_problem_get_number_of_objectives(PROBLEM),
                        coco_problem_get_smallest_values_of_interest(PROBLEM),
                        coco_problem_get_largest_values_of_interest(PROBLEM),
                        (size_t) evaluations_remaining,
                        random_generator,
                        titlestr);
      }
      else if(ALGORITHM == 1){

      }

      /* Break the loop if the algorithm performed no evaluations or an unexpected thing happened */
      if (coco_problem_get_evaluations(PROBLEM) == evaluations_done) {
        printf("WARNING: Budget has not been exhausted (%lu/%lu evaluations done)!\n",
            (unsigned long) evaluations_done, (unsigned long) dimension * BUDGET_MULTIPLIER);
        break;
      }
      else if (coco_problem_get_evaluations(PROBLEM) < evaluations_done)
        coco_error("Something unexpected happened - function evaluations were decreased!");
    }
    /* Keep track of time */
    timing_data_time_problem(timing_data, PROBLEM);
  }

  /* Output and finalize the timing data */
  timing_data_finalize(timing_data);

  coco_observer_free(observer);
  coco_suite_free(suite);
}

/**
 * A simple example of benchmarking random search on a given suite with default instances
 * that can serve also as a timing experiment.
 *
 * @param suite_name Name of the suite (e.g. "bbob" or "bbob-biobj").
 * @param suite_options Options of the suite (e.g. "dimensions: 2,3,5,10,20 instance_indices: 1-5").
 * @param observer_name Name of the observer matching with the chosen suite (e.g. "bbob-biobj"
 * when using the "bbob-biobj-ext" suite).
 * @param observer_options Options of the observer (e.g. "result_folder: folder_name")
 * @param random_generator The random number generator
 */
void my_example_experiment(const char *file_name,
                        coco_random_state_t *random_generator) {
  FILE *fp;
  printf("generating problem...\n");
  MY_PROBLEM *my_problem = init_problem(random_generator);
  printf("success generation\n");
  // for(size_t i = 0; i < NUMBER_OF_PROBLEM; i++){
  //   printf("%s:dimension%ld:instance%ld:range[0,%.0f]\n",my_problem[i].function_name, my_problem[i].dimension, my_problem[i].instance, my_problem[i].largest[0]);
  //   printf("optimal solution:");
  //   for(size_t j = 0; j < my_problem[i].dimension; j++){
  //     printf("%lf ", my_problem[i].optimal[j]);
  //   }
  //   printf("\n");
  //   printf("target hit result:\n");
  //   for(size_t j = 0; j < NUMBER_OF_TARGET; j++){
  //     printf("%.2e:%d\n", target[j], my_problem[i].evaluate_result[j]);
  //   }
  // }
  /* Iterate over all problems in the suite */
  for(size_t i = 0; i < NUMBER_OF_PROBLEM; i++){
    const char *function_name = my_problem[i].function_name;
    size_t dimension = my_problem[i].dimension;
    double amount = 0;
    size_t target_count = 0;
    //filename select
    char titlestr[128] = "./output/";
    char num[30] = "";
    if(ALGORITHM == 0){
      strcat(titlestr,"de/");
    }
    else if(ALGORITHM == 1){
      strcat(titlestr,"ga/");
    }
    strcat(titlestr,function_name);
    sprintf(num, "/%d", (int)my_problem[i].r);
    strcat(titlestr,num);
    if((int)my_problem[i].largest[0] == 1){
      sprintf(num, "/6/");
    }
    else{
      sprintf(num, "/%d/", (int)my_problem[i].largest[0]);
    }
    strcat(titlestr,num);
    sprintf(num, "%ld", dimension);
    strcat(titlestr,num);
    if(ENCODING == 0){
      if(APPROACH == 0){
        strcat(titlestr,"d/L-");
      }
      else{
        strcat(titlestr,"d/B-");
      }
    }
    else if(ENCODING == 1){
      if(APPROACH == 0){
        strcat(titlestr,"d/U-Lf-");
      }
      else if(APPROACH == 1){
        strcat(titlestr,"d/U-Lm-");
      }
      else if(APPROACH == 2){
        strcat(titlestr,"d/U-Lb-");
      }
      else if(APPROACH == 3){
        strcat(titlestr,"d/U-B-");
      }
    }
    else if(ENCODING == 2){
      if(APPROACH == 0){
        strcat(titlestr,"d/U2-L-");
      }
      else{
        strcat(titlestr,"d/U2-B-");
      }
    }
    sprintf(num, "%ld", my_problem[i].instance);
    strcat(titlestr,num);
    strcat(titlestr,".txt");
    //printf("%s\n",titlestr);
    /* Run the algorithm at least once */
    for (size_t run = 1; run <= 1 + INDEPENDENT_RESTARTS; run++) {
      long evaluations_done = my_problem[i].evaluation_cnt;
      long evaluations_remaining = (long) (dimension * BUDGET_MULTIPLIER) - evaluations_done;
      /* Break the loop if the target was hit or there are no more remaining evaluations */
      if((evaluations_remaining <= 0)){
        break;
      }
      // if(my_problem[i].dimension != 5 && my_problem[i].function_name != "f13"){
      //   break;
      // }
      /* Call the optimization algorithm for the remaining number of evaluations */
      if(ALGORITHM == 0){
        my_de_nopcm(function_name,
                        dimension,
                        1,
                        my_problem[i].smallest,
                        my_problem[i].largest,
                        (size_t) evaluations_remaining,
                        random_generator,
                        titlestr,
                        &my_problem[i]);
      }
      else if(ALGORITHM == 1){

      }
    }
    // if(my_problem[i].dimension == 5 && strcmp(my_problem[i].function_name, "f13") == 0){
     // printf("%s:dimension%ld:instance%ld:range[0,%.0f]:integer ratio%ld/5\n",my_problem[i].function_name, dimension, my_problem[i].instance, my_problem[i].largest[0], my_problem[i].r);
    //   printf("optimal solution:");
    //   for(size_t j = 0; j < dimension; j++){
    //     printf("%lf ", my_problem[i].optimal[j]);
    //   }
    //   printf("\n");

    //   printf("best solution   :");
    //   for(size_t j = 0; j < dimension; j++){
    //     printf("%lf ", my_problem[i].best_solution[j]);
    //   }
    //   printf("\n");

      fp = fopen(titlestr, "w");
      while(1){
        if(amount > 4){
          break;
        }
        for(int k = 0; k < NUMBER_OF_TARGET; k++){
          if(((double)dimension*pow(10, amount) >= (double)(double)my_problem[i].evaluate_result[k] ) && (my_problem[i].evaluate_result[k] != -1)){
            target_count++;
          }
          else{
            break;
          }
        }
        fprintf(fp,"%f %ld\n", amount, target_count);
        target_count = 0;
        amount += 0.001;
      }
      fclose(fp);
    //}
  }
  for (size_t i = 0; i < NUMBER_OF_PROBLEM; i++) {
    free_problem(&my_problem[i]);
  }
  free(my_problem);  // 問題配列自体の解放
}

// MyCOCO
void free_problem(MY_PROBLEM* problem) {
    // function_name のメモリ解放
    if (problem->function_name) {
        free(problem->function_name);
    }

    // smallest 配列のメモリ解放
    if (problem->smallest) {
        free(problem->smallest);
    }

    // largest 配列のメモリ解放
    if (problem->largest) {
        free(problem->largest);
    }

    // optimal 配列のメモリ解放
    if (problem->optimal) {
        free(problem->optimal);
    }

    // best_solution 配列のメモリ解放
    if (problem->best_solution) {
        free(problem->best_solution);
    }
}

MY_PROBLEM* init_problem(coco_random_state_t *random_generator){
  MY_PROBLEM* problems = (MY_PROBLEM*)malloc( NUMBER_OF_PROBLEM * sizeof(MY_PROBLEM));
  if (!problems) {
      fprintf(stderr, "Memory allocation failed for MY_PROBLEM.\n");
      exit(EXIT_FAILURE);
  }

  // 各問題を初期化(func)
  char *function[] = {"f1", "f3", "f8"};
  size_t dimension[] = {5, 10, 20, 40, 80, 160};
  double range[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
  double coco_range[] = {1, 3, 7, 15};
  int problem_cnt = 0;
  double amount = -2;
  for(size_t func_cnt = 0; func_cnt < sizeof(function) / sizeof(function[0]); func_cnt++){
    for(size_t r_cnt = 1; r_cnt < 5; r_cnt++){
      for(size_t range_cnt = 0; range_cnt < sizeof(range) / sizeof(range[0]); range_cnt++){
        for(size_t dimension_cnt = 0; dimension_cnt < sizeof(dimension) / sizeof(dimension[0]); dimension_cnt++){
          for (size_t instance_count = 0; instance_count < 15; instance_count++){
            problems[problem_cnt].function_name = (char*)malloc(strlen(function[func_cnt]) + 1);
            if (!problems[problem_cnt].function_name) {
                fprintf(stderr, "Memory allocation failed for function_name.\n");
                exit(EXIT_FAILURE);
            }
            strcpy(problems[problem_cnt].function_name, function[func_cnt]);

            problems[problem_cnt].dimension = dimension[dimension_cnt];
            problems[problem_cnt].instance = instance_count;
            problems[problem_cnt].evaluation_cnt = 0;
            problems[problem_cnt].end_flag = 0;
            problems[problem_cnt].r = r_cnt;
            for(size_t i = 0; i < NUMBER_OF_TARGET; i++){
              problems[problem_cnt].evaluate_result[i] = -1;
            }
            // optimalとsmallest と largest のメモリを確保
            problems[problem_cnt].smallest = (double*)malloc(dimension[dimension_cnt] * sizeof(double));
            problems[problem_cnt].largest = (double*)malloc(dimension[dimension_cnt] * sizeof(double));
            problems[problem_cnt].optimal = (double*)malloc(dimension[dimension_cnt] * sizeof(double));
            problems[problem_cnt].best_solution = (double*)malloc(dimension[dimension_cnt] * sizeof(double));
            
            if (!problems[problem_cnt].smallest || !problems[problem_cnt].largest) {
              fprintf(stderr, "Memory allocation failed for arrays.\n");
              exit(EXIT_FAILURE);
            }
            for(size_t i = 0; i < dimension[dimension_cnt]; i++){
              problems[problem_cnt].best_solution[i] = 100;
            }

            //if(range_cnt != 4){
              // 配列を初期化
              for (size_t j = 0; j < dimension[dimension_cnt]*r_cnt/5; j++) {
                problems[problem_cnt].smallest[j] = 0;
                problems[problem_cnt].largest[j] = range[range_cnt];
                problems[problem_cnt].optimal[j] = (int)(coco_random_uniform(random_generator) * (problems[problem_cnt].largest[j] - problems[problem_cnt].smallest[j] + 1) + problems[problem_cnt].smallest[j]);
              }
              for (size_t j = dimension[dimension_cnt]*r_cnt/5; j < dimension[dimension_cnt]; j++) {
                problems[problem_cnt].smallest[j] = -5;
                problems[problem_cnt].largest[j] = 5;
                problems[problem_cnt].optimal[j] = problems[problem_cnt].smallest[j] + coco_random_uniform(random_generator) * (problems[problem_cnt].largest[j] - problems[problem_cnt].smallest[j]);
              }
            // }
            // else{
            //   // 配列を初期化
            //   for (size_t m = 0; m < 4; m++) {
            //     for (size_t j = m*dimension[dimension_cnt]/5; j < (m + 1)*dimension[dimension_cnt]/5; j++) {
            //       problems[problem_cnt].smallest[j] = 0;
            //       problems[problem_cnt].largest[j] = coco_range[m];
            //       problems[problem_cnt].optimal[j] = (int)(coco_random_uniform(random_generator) * (problems[problem_cnt].largest[j] - problems[problem_cnt].smallest[j] + 1) + problems[problem_cnt].smallest[j]);
            //     }
            //   }
            //   for (size_t j = 4*dimension[dimension_cnt]/5; j < (4 + 1)*dimension[dimension_cnt]/5; j++) {
            //     problems[problem_cnt].smallest[j] = -5;
            //     problems[problem_cnt].largest[j] = 5;
            //     problems[problem_cnt].optimal[j] = problems[problem_cnt].smallest[j] + coco_random_uniform(random_generator) * (problems[problem_cnt].largest[j] - problems[problem_cnt].smallest[j]);
            //   }
            // }

            problem_cnt++;
          }
        }
      }
    }
  }
  for(int target_cnt = 0; target_cnt < NUMBER_OF_TARGET; target_cnt++){
    target[target_cnt] = pow(10, -amount);
    amount += 0.2;
  }

  return problems;
}

//EA_DEFAULT_PARTS
void ea_group_initialization(double** population, size_t dimension, const double* lower_bounds, const double* upper_bounds, coco_random_state_t *random_generator){
  //initialization
  for (int i = 0; i < DE_N; i++) {
    for (int j = 0; j < dimension; j++) {
      if(ENCODING == 0){
        double range = upper_bounds[j] - lower_bounds[j];
        population[i][j] = lower_bounds[j] + coco_random_uniform(random_generator) * range;
      }
      else if(ENCODING == 1){
        population[i][j] = coco_random_uniform(random_generator);
      }
      else if(ENCODING == 2){
        if(lower_bounds[j] == -5){
          double range = upper_bounds[j] - lower_bounds[j];
          population[i][j] = lower_bounds[j] + coco_random_uniform(random_generator) * range;
        }
        else{
          double range = (upper_bounds[j] + 0.5 - FLT_EPSILON) - (lower_bounds[j] - 0.5);
          population[i][j] = lower_bounds[j] - 0.5 + coco_random_uniform(random_generator) * range;
        }
      }
    }
  }
}

void ea_group_encoding(double** x, double** tmp, size_t dimension, const double* lower_bounds, const double* upper_bounds){
  if((ENCODING == 0 && APPROACH == 1) || (ENCODING == 1 && APPROACH == 3) || (ENCODING == 2 && APPROACH == 1)){
    for (int i = 0; i < DE_N; i++) {
      for(int j = 0; j < dimension; j++){
        tmp[i][j] = x[i][j];
      }
    }
  }
  for (int i = 0; i < DE_N; i++) {
    if(ENCODING == 0 || ENCODING == 2){
      if(APPROACH == 0){
        round_vec(x[i],dimension,lower_bounds,upper_bounds);
        for(int j = 0; j < dimension; j++){
          tmp[i][j] = x[i][j];
        }
      }
      else if(APPROACH == 1){
        round_vec(tmp[i],dimension,lower_bounds,upper_bounds);
      }
    }
    else{
      if(APPROACH == 0 || APPROACH  == 1 || APPROACH  == 2){
        new_round_vec(x[i], dimension, lower_bounds, upper_bounds);
        for(int j = 0; j < dimension; j++){
          tmp[i][j] = x[i][j];
        }
        decoding_vec(tmp[i], dimension, lower_bounds, upper_bounds);
      }
      else{
        decoding_vec(tmp[i], dimension, lower_bounds, upper_bounds);
      }
    }
  }
}

void round_vec(double *x, size_t dimention_size, const double *lower_bounds, const double *upper_bounds){
  double y[40] = {0};
  double y_star;
  double min_dist = 0;
  for(int i = 0; i < dimention_size; i++){
    if(lower_bounds[i] != -5){
      //補助値を計算
      for(int  j = 0; j <= (int)upper_bounds[i]; j++){
          y[j] = j;
      }
      //連続値に最も近い補助値 y^* を求める
      min_dist = fabs(fabs(y[0]) - fabs(x[i]));
      y_star = y[0];

      for (int j = 1; j <= (int)upper_bounds[i]; j++) {
          double dist = fabs(fabs(y[j]) - fabs(x[i]));;
          if (dist <= min_dist) {
              min_dist = dist;
              y_star = y[j];
          }
      }
      x[i] = y_star;
    }
  }
}

void new_round_vec(double *x, size_t dimention_size, const double *lower_bounds, const double *upper_bounds){
  double y[40]= {0};

  for(int i = 0; i < dimention_size; i++){
    if(lower_bounds[i] != -5){
      // 補助値を計算
      for(int j = 0; j <= (int)upper_bounds[i] + 1; j++){
        y[j] = 1/(upper_bounds[i] + 1) * j;
      }

      //整数型の丸め
      if(APPROACH == 0){//U-Lf
        for(int j = 1; j <= (int)upper_bounds[i] + 1; j++){
          if(x[i] < y[j]){
            if(j == 1){
              x[i] = 0;
            }
            else if(j == (int)upper_bounds[i] + 1){
              x[i] = 1;
            }
            else{
              x[i] = y[j - 1] + 1/((upper_bounds[i] + 1)*2);
            }
            break;
          }
        }
      }
      else if(APPROACH == 1){//U-Lm
        for(int j = 1; j <= (int)upper_bounds[i] + 1; j++){
          if(x[i] < y[j]){
            x[i] = y[j - 1] + 1/((upper_bounds[i] + 1)*2);
            break;
          }
        }
      }
      else if(APPROACH == 2){//U-Lb
        for(int j = 1; j <= (int)upper_bounds[i] + 1; j++){
          if(x[i] < y[j]){
            if(fabs(x[i] - y[j]) < fabs(x[i] - y[j - 1])){
              x[i] = y[j] - FLT_EPSILON;
            }
            else{
              x[i] = y[j - 1];
            }
            break;
          }
        }
      }
    }
  }
}

void decoding_vec(double *x, size_t dimention_size, const double *lower_bounds, const double *upper_bounds){
  for(int i = 0; i < dimention_size; i++){
    if(lower_bounds[i] != -5){
      x[i] = floor(x[i]*(upper_bounds[i] + 1));
      if(x[i] > upper_bounds[i]){
        x[i] = upper_bounds[i];
      }
    }
    else{
      x[i] =  10.0 * x[i] - 5.0;
    }
  }
}

void ea_sd_calc(double* sum, double* sum2, double** tmp, size_t dimension, FILE *fp){
  for (int j = 0; j < dimension; j++) {
    sum[j] = 0;
    sum2[j] = 0;
  }
  //heikin
  for(int i = 0; i < DE_N; i++){
    for(int j = 0; j < dimension; j++){
        sum[j] += tmp[i][j];
    }
  }
  for(int j = 0; j < dimension; j++){
    sum[j] = sum[j]/DE_N;
  }
  //bunsan
  for(int j = 0; j < dimension; j++){
    for(int i = 0; i < DE_N; i++){
      sum2[j] += (tmp[i][j] - sum[j])*(tmp[i][j] - sum[j]);
    }
  }
  //hyoujyunhensa
  for(int j = 0; j < dimension; j++){
    fprintf(fp,"%.30lf ",sqrt(sum2[j]/DE_N));
  }
  fprintf(fp,"\n");
}

//COCO
/**
 * Allocates memory for the timing_data_t object and initializes it.
 */
static timing_data_t *timing_data_initialize(coco_suite_t *suite) {

	timing_data_t *timing_data = (timing_data_t *) coco_allocate_memory(sizeof(*timing_data));
	size_t function_idx, dimension_idx, instance_idx, i;

	/* Find out the number of all dimensions */
	coco_suite_decode_problem_index(suite, coco_suite_get_number_of_problems(suite) - 1, &function_idx,
			&dimension_idx, &instance_idx);
	timing_data->number_of_dimensions = dimension_idx + 1;
	timing_data->current_idx = 0;
	timing_data->output = (char **) coco_allocate_memory(timing_data->number_of_dimensions * sizeof(char *));
	for (i = 0; i < timing_data->number_of_dimensions; i++) {
		timing_data->output[i] = NULL;
	}
	timing_data->previous_dimension = 0;
	timing_data->cumulative_evaluations = 0;
	time(&timing_data->start_time);
	time(&timing_data->overall_start_time);

	return timing_data;
}

/**
 * Keeps track of the total number of evaluations and elapsed time. Produces an output string when the
 * current problem is of a different dimension than the previous one or when NULL.
 */
static void timing_data_time_problem(timing_data_t *timing_data, coco_problem_t *problem) {

	double elapsed_seconds = 0;

	if ((problem == NULL) || (timing_data->previous_dimension != coco_problem_get_dimension(problem))) {

		/* Output existing timing information */
		if (timing_data->cumulative_evaluations > 0) {
			time_t now;
			time(&now);
			elapsed_seconds = difftime(now, timing_data->start_time) / (double) timing_data->cumulative_evaluations;
			timing_data->output[timing_data->current_idx++] = coco_strdupf("d=%lu done in %.2e seconds/evaluation\n",
					timing_data->previous_dimension, elapsed_seconds);
		}

		if (problem != NULL) {
			/* Re-initialize the timing_data */
			timing_data->previous_dimension = coco_problem_get_dimension(problem);
			timing_data->cumulative_evaluations = coco_problem_get_evaluations(problem);
			time(&timing_data->start_time);
		}

	} else {
		timing_data->cumulative_evaluations += coco_problem_get_evaluations(problem);
	}
}

/**
 * Outputs and finalizes the given timing data.
 */
static void timing_data_finalize(timing_data_t *timing_data) {

	/* Record the last problem */
	timing_data_time_problem(timing_data, NULL);

  if (timing_data) {
  	size_t i;
  	double elapsed_seconds;
		time_t now;
		int hours, minutes, seconds;

		time(&now);
		elapsed_seconds = difftime(now, timing_data->overall_start_time);

  	printf("\n");
  	for (i = 0; i < timing_data->number_of_dimensions; i++) {
    	if (timing_data->output[i]) {
				printf("%s", timing_data->output[i]);
				coco_free_memory(timing_data->output[i]);
    	}
    }
  	hours = (int) elapsed_seconds / 3600;
  	minutes = ((int) elapsed_seconds % 3600) / 60;
  	seconds = (int)elapsed_seconds - (hours * 3600) - (minutes * 60);
  	printf("Total elapsed time: %dh%02dm%02ds\n", hours, minutes, seconds);

    coco_free_memory(timing_data->output);
    coco_free_memory(timing_data);
  }
}

// the function is able to get coco-function number
char *get_short_function_number(const char *problem_name) {
    const char *f_position = strstr(problem_name, "f");
    if (f_position == NULL) {
        return NULL;  // "f"が見つからない場合はNULLを返す
    }

    int number = atoi(f_position + 1); // "f"の後ろから整数部分を取得
    char *short_name = (char *)malloc(10 * sizeof(char));
    if (short_name == NULL) {
        return NULL;  // メモリ割り当てに失敗した場合はNULLを返す
    }

    sprintf(short_name, "f%d", number); // 短縮形式の"f1"や"f2"を生成
    return short_name;
}

//EA algorithm
void de_nopcm(evaluate_function_t evaluate_func,
                      const size_t dimension,
                      const size_t number_of_objectives,
                      const double *lower_bounds,
                      const double *upper_bounds,
                      const size_t max_budget,
                      coco_random_state_t *random_generator,
                      char *titlestr){
  double **population = (double**)malloc(DE_N * sizeof(double*));
  double *functions_values = coco_allocate_vector(number_of_objectives);
  double **trial = (double**)malloc(DE_N * sizeof(double*));
  double *mutate = coco_allocate_vector(dimension);
  double **tmp = (double**)malloc(DE_N * sizeof(double*));
  double *rnd_vals = coco_allocate_vector(dimension);
  size_t evaluation = 0;
  size_t i, j;
  int vector[3];
  double value_population[DE_N];
  double value_trial[DE_N];
  //FILE *fp;
  double *sum = coco_allocate_vector(dimension); 
  double *sum2 = coco_allocate_vector(dimension); 
  int output_cnt = 0;
  //fp = fopen(titlestr, "w");

  for (i = 0; i < DE_N; i++) {
        population[i] = coco_allocate_vector(dimension);
        trial[i] = coco_allocate_vector(dimension);
        tmp[i] = coco_allocate_vector(dimension);
        if (population[i] == NULL) {
            printf("メモリの確保に失敗しました。\n");
        }
  }
  //initialization
  ea_group_initialization(population, dimension, lower_bounds, upper_bounds, random_generator);
  //encoding
  ea_group_encoding(population, tmp, dimension, lower_bounds, upper_bounds);
  //evaluation
  for (i = 0; i < DE_N; i++) {
    evaluate_func(tmp[i], functions_values);
    evaluation++;
    value_population[i] = functions_values[0];
  }
  //hanpuku
  while(evaluation  < max_budget){
    //hyoujyunhensa+output
    if(output_cnt == 0){
      //ea_sd_calc(sum, sum2, tmp, dimension, fp);
    }
    output_cnt++;
    if(output_cnt == dimension){
      output_cnt = 0;
    }

    for (i = 0; i < DE_N; i++) {
      //selection
      vector[0] = (int)(coco_random_uniform(random_generator)*DE_N);
      do {
          vector[1] = (int)(coco_random_uniform(random_generator)*DE_N);
      } while (vector[1] == vector[0]);

      do {
          vector[2] = (int)(coco_random_uniform(random_generator)*DE_N);
      } while (vector[2] == vector[0] || vector[2] == vector[1]);
      //mutation
      for (j = 0; j < dimension; j++) {
        mutate[j] = population[vector[0]][j] + DE_F * (population[vector[1]][j] - population[vector[2]][j]);
        if(ENCODING== 0){
          if (mutate[j] < lower_bounds[j]){
            mutate[j] = (lower_bounds[j] + population[i][j]) / 2.0;
          }
          else if(mutate[j] > upper_bounds[j]){
            mutate[j] = (upper_bounds[j] + population[i][j]) / 2.0;
          }
        }
        else if(ENCODING == 1){
          if (mutate[j] < 0){
            mutate[j] = (population[i][j]) / 2.0;
          }
          else if(mutate[j] > 1){
            mutate[j] = (1 + population[i][j]) / 2.0;
          }
        }
        else if(ENCODING == 2){
          if (mutate[j] < lower_bounds[j] - 0.5){
            mutate[j] = (lower_bounds[j] - 0.5 + population[i][j]) / 2.0;
          }
          else if(mutate[j] > upper_bounds[j] + 0.5 - FLT_EPSILON){
            mutate[j] = (upper_bounds[j] + 0.5 - FLT_EPSILON + population[i][j]) / 2.0;
          }
        }
      }
      //crossover
      int j_rand = (int)(coco_random_uniform(random_generator)*(int)dimension);

      // Generate random values between 0 and 1
      for (j = 0; j < dimension; j++) {
          rnd_vals[j] = coco_random_uniform(random_generator);
      }
      // Set rnd_vals[j_rand] to 0.0
      rnd_vals[j_rand] = 0.0;
      // Perform binomial crossover
      for (j = 0; j < dimension; j++) {
          if (rnd_vals[j] <= DE_CR) {
              trial[i][j] = mutate[j];
          } else {
              trial[i][j] = population[i][j];
          }
      }
    }
    //encoding
    ea_group_encoding(trial, tmp, dimension, lower_bounds, upper_bounds);
    //evaluation
    for (i = 0; i < DE_N; i++) {
      evaluate_func(tmp[i], functions_values);
      evaluation++;
      value_trial[i] = functions_values[0];
    }

    //enviroment selection
    for(i = 0; i < DE_N; i++){
      if(value_trial[i] <= value_population[i]){
        for (j = 0; j < dimension; j++) {
          population[i][j] = trial[i][j];
        }
        value_population[i] = value_trial[i];
      }
    }
  }
  //next instance
  instance_cnt++;
  if(instance_cnt == 15){
    instance_cnt = 0;
  }
  //fclose(fp);
  //memory free
  for (i = 0; i < DE_N; ++i) {
    coco_free_memory(population[i]);
    coco_free_memory(trial[i]);
    coco_free_memory(tmp[i]);
  }
  free(population);
  coco_free_memory(functions_values);
  coco_free_memory(mutate);
  coco_free_memory(tmp);
  coco_free_memory(rnd_vals);
  coco_free_memory(sum);
  coco_free_memory(sum2);
}

int find_min_index(double value_population[]) {
  int min_index = 0;  // 最初の要素を最小値と仮定
  for (int i = 1; i < DE_N; i++) {
      if (value_population[i] < value_population[min_index]) {
          min_index = i;  // 最小値を更新
      }
  }
  return min_index;
}

void my_de_nopcm(const char* function_name,
                      const size_t dimension,
                      const size_t number_of_objectives,
                      const double *lower_bounds,
                      const double *upper_bounds,
                      const size_t max_budget,
                      coco_random_state_t *random_generator,
                      char *titlestr,
                      MY_PROBLEM *problem){
  double **population = (double**)malloc(DE_N * sizeof(double*));
  double *functions_values = coco_allocate_vector(number_of_objectives);
  double *tmp_functions_values = coco_allocate_vector(number_of_objectives);
  double **trial = (double**)malloc(DE_N * sizeof(double*));
  double *mutate = coco_allocate_vector(dimension);
  double **tmp = (double**)malloc(DE_N * sizeof(double*));
  double *rnd_vals = coco_allocate_vector(dimension);
  int evaluation = 0;
  size_t i, j;
  int vector[3];
  int min_pos = 0;
  double value_population[DE_N];
  double value_trial[DE_N];
  //FILE *fp;
  double *sum = coco_allocate_vector(dimension); 
  double *sum2 = coco_allocate_vector(dimension); 
  int output_cnt = 0;
  //fp = fopen(titlestr, "w");
  for (i = 0; i < DE_N; i++) {
        population[i] = coco_allocate_vector(dimension);
        trial[i] = coco_allocate_vector(dimension);
        tmp[i] = coco_allocate_vector(dimension);
        if (population[i] == NULL) {
            printf("メモリの確保に失敗しました。\n");
        }
  }
  //initialization
  ea_group_initialization(population, dimension, lower_bounds, upper_bounds, random_generator);
  
  // for(i = 0; i < DE_N; i++){
  //   for(j = 0; j < dimension; j++){
  //     printf("%.30lf ", population[i][j]);
  //   }
  //   printf("\n");
  // }
  
  //encoding
  ea_group_encoding(population, tmp, dimension, lower_bounds, upper_bounds);
  //evaluation
  for (i = 0; i < DE_N; i++) {
    my_evaluate_func(tmp[i], functions_values, function_name, dimension, problem->optimal);
    evaluation++;
    value_population[i] = functions_values[0];
  }
  min_pos =  find_min_index(value_population);
  for(size_t target_cnt = 0; target_cnt < NUMBER_OF_TARGET; target_cnt++){
    if(target[target_cnt] > value_population[min_pos]){
      if(problem->evaluate_result[target_cnt] == -1){
        problem->evaluate_result[target_cnt] = evaluation;
      }
      else if(problem->evaluate_result[target_cnt] > evaluation){
        problem->evaluate_result[target_cnt] = evaluation;
      }
      if(target_cnt == NUMBER_OF_TARGET - 1){
        problem->end_flag = 1;
      }
    }
  }
  //hanpuku
  while(evaluation  < max_budget){
    if(problem->end_flag == 1){
      break;
    }

    //hyoujyunhensa+output
    if(output_cnt == 0){
      //ea_sd_calc(sum, sum2, tmp, dimension, fp);
    }
    output_cnt++;
    if(output_cnt == dimension){
      output_cnt = 0;
    }

    for (i = 0; i < DE_N; i++) {
      //selection
      vector[0] = (int)(coco_random_uniform(random_generator)*DE_N);
      do {
          vector[1] = (int)(coco_random_uniform(random_generator)*DE_N);
      } while (vector[1] == vector[0]);

      do {
          vector[2] = (int)(coco_random_uniform(random_generator)*DE_N);
      } while (vector[2] == vector[0] || vector[2] == vector[1]);
      //mutation
      for (j = 0; j < dimension; j++) {
        mutate[j] = population[vector[0]][j] + DE_F * (population[vector[1]][j] - population[vector[2]][j]);
        if(ENCODING== 0){
          if (mutate[j] < lower_bounds[j]){
            mutate[j] = (lower_bounds[j] + population[i][j]) / 2.0;
          }
          else if(mutate[j] > upper_bounds[j]){
            mutate[j] = (upper_bounds[j] + population[i][j]) / 2.0;
          }
        }
        else if(ENCODING == 1){
          if (mutate[j] < 0){
            mutate[j] = (population[i][j]) / 2.0;
          }
          else if(mutate[j] > 1){
            mutate[j] = (1 + population[i][j]) / 2.0;
          }
        }
        else if(ENCODING == 2){
          if (mutate[j] < lower_bounds[j] - 0.5){
            mutate[j] = (lower_bounds[j] - 0.5 + population[i][j]) / 2.0;
          }
          else if(mutate[j] > upper_bounds[j] + 0.5 - FLT_EPSILON){
            mutate[j] = (upper_bounds[j] + 0.5 - FLT_EPSILON + population[i][j]) / 2.0;
          }
        }
      }
      //crossover
      int j_rand = (int)(coco_random_uniform(random_generator)*(int)dimension);

      // Generate random values between 0 and 1.
      for (j = 0; j < dimension; j++) {
          rnd_vals[j] = coco_random_uniform(random_generator);
      }
      // Set rnd_vals[j_rand] to 0.0
      rnd_vals[j_rand] = 0.0;
      // Perform binomial crossover
      for (j = 0; j < dimension; j++) {
          if (rnd_vals[j] <= DE_CR) {
              trial[i][j] = mutate[j];
          } else {
              trial[i][j] = population[i][j];
          }
      }
    }
    //encoding
    ea_group_encoding(trial, tmp, dimension, lower_bounds, upper_bounds);
    //evaluation
    for (i = 0; i < DE_N; i++) {
      my_evaluate_func(tmp[i], functions_values, function_name, dimension, problem->optimal);
      evaluation++;
      value_trial[i] = functions_values[0];
    }

    //enviroment selection
    for(i = 0; i < DE_N; i++){
      if(value_trial[i] <= value_population[i]){
        for (j = 0; j < dimension; j++) {
          population[i][j] = trial[i][j];
        }
        value_population[i] = value_trial[i];
      }
    }

    min_pos =  find_min_index(value_population);
    for(size_t target_cnt = 0; target_cnt < NUMBER_OF_TARGET; target_cnt++){
      if(target[target_cnt] > value_population[min_pos]){
        if(problem->evaluate_result[target_cnt] == -1){
          problem->evaluate_result[target_cnt] = evaluation;
        }
        else{
          if(problem->evaluate_result[target_cnt] > evaluation){
            problem->evaluate_result[target_cnt] = evaluation;
          }
        }
        if(target_cnt == NUMBER_OF_TARGET - 1){
          problem->end_flag = 1;
        }
      }
    }
    // for(i = 0; i < DE_N; i++){
    //   for(j = 0; j < dimension; j++){
    //     printf("%.30lf ", population[i][j]);
    //   }
    //   printf("\n");
    // }
    // printf("%d\n", evaluation);
  }

  my_evaluate_func(problem->best_solution, functions_values, function_name, dimension, problem->optimal);
  
  my_evaluate_func(population[min_pos], tmp_functions_values, function_name, dimension, problem->optimal);
  // printf("best_solution:");
  if(functions_values[0] > tmp_functions_values[0]){
    for(i = 0; i < dimension; i++){
      problem->best_solution[i] = population[min_pos][i];
    }
  }

  problem->end_flag = 0;
  problem->evaluation_cnt += evaluation;
  //fclose(fp);
  //memory free
  for (i = 0; i < DE_N; ++i) {
    coco_free_memory(population[i]);
    coco_free_memory(trial[i]);
    coco_free_memory(tmp[i]);
  }
  free(population);
  coco_free_memory(functions_values);
  coco_free_memory(tmp_functions_values);
  coco_free_memory(mutate);
  coco_free_memory(tmp);
  coco_free_memory(rnd_vals);
  coco_free_memory(sum);
  coco_free_memory(sum2);
}

//MY_EVALUATE_FUNC
// void f1(const double *x, double *y, size_t dimension, double* optimal) {//sphere
//   y[0] = 0;
//   for(size_t i = 0; i < dimension; i++){
//     y[0] += (optimal[i] - x[i])*(optimal[i] - x[i]);
//   }
// }

// void f2(const double *x, double *y, size_t dimension, double* optimal) {//sphere
//   y[0] = 0;
//   for(size_t i = 0; i < dimension; i++){
//     y[0] += pow(10, 6*i/(dimension - 1)) * (optimal[i] - x[i])*(optimal[i] - x[i]);
//   }

//   y[0] = pow(10, -3) * y[0];
// }

// void f3(const double *x, double *y, size_t dimension, double* optimal) {//rastrigin
//   y[0] = 0;
//   for(size_t i = 0; i < dimension; i++){
//     y[0] += (optimal[i] - x[i])*(optimal[i] - x[i]) - 10*cos(2*M_PI*(optimal[i] - x[i]));
//   }

//   y[0] += 10*(double)dimension;

//   y[0] = 0.1 * y[0];
// }

// void f8(const double *x, double *y, size_t dimension, double* optimal) {//rosenbrock
//   y[0] = 0;
//   for(size_t i = 0; i < dimension - 1; i++){
//     y[0] += 100*((optimal[i + 1] - x[i + 1]) - (optimal[i] - x[i]) * (optimal[i] - x[i]))*((optimal[i + 1] - x[i + 1]) - (optimal[i] - x[i]) * (optimal[i] - x[i])) + (optimal[i] - x[i] - 1)*(optimal[i] - x[i] - 1);
//   }
// }

static double f_sphere_raw(const double *x, const size_t number_of_variables, const double *opt) {
  size_t i = 0;
  double result = 0.0;
  double shifted_x;

  for (i = 0; i < number_of_variables; ++i) {
    shifted_x = x[i] - opt[i];
    result += shifted_x * shifted_x;
  }

  return result;
}

static double f_ellipsoid_raw(const double *x, const size_t number_of_variables, const double *opt) {
  static const double condition = 1.0e6;
  size_t i = 0;
  double result;
  double shifted_x;

  result = (x[0] - opt[0]) * (x[0] - opt[0]);
  for (i = 1; i < number_of_variables; ++i) {
    double exponent = 1.0 * (double)(long)i / ((double)(long)number_of_variables - 1.0);
    shifted_x = x[i] - opt[i];
    result += pow(condition, exponent) * shifted_x * shifted_x;
  }

  return result;
}

static double f_rastrigin_raw(const double *x, const size_t number_of_variables, const double *opt) {
  size_t i = 0;
  double result;
  double sum1 = 0.0, sum2 = 0.0;
  double shifted_x;

  for (i = 0; i < number_of_variables; ++i) {
    shifted_x = x[i] - opt[i];
    sum1 += cos(coco_two_pi * shifted_x);
    sum2 += shifted_x * shifted_x;
  }

  if (sum2 > 1e22) /* cos(inf) -> nan */
    return sum2;

  result = 10.0 * ((double)(long)number_of_variables - sum1) + sum2;

  return result;
}

static double f_rosenbrock_raw(const double *x, const size_t number_of_variables, const double *opt) {
  size_t i = 0;
  double result;
  double s1 = 0.0, s2 = 0.0, tmp;

  for (i = 0; i < number_of_variables - 1; ++i) {
    tmp = ((x[i] - opt[i]) * (x[i] - opt[i]) - (x[i + 1] - opt[i + 1]));
    s1 += tmp * tmp;
    tmp = (x[i] - opt[i] - 1.0);
    s2 += tmp * tmp;
  }
  result = 100.0 * s1 + s2;

  return result;
}

static double f_bent_cigar_raw(const double *x, const size_t number_of_variables, const double *opt) {
  static const double condition = 1.0e6;
  size_t i;
  double result;
  double shifted_x;

  result = (x[0] - opt[0]) * (x[0] - opt[0]);
  for (i = 1; i < number_of_variables; ++i) {
    shifted_x = x[i] - opt[i];
    result += condition * shifted_x * shifted_x;
  }

  return result;
}

static double f_sharp_ridge_raw(const double *x, const size_t number_of_variables, const double *opt) {
  static const double alpha = 100.0;
  const double d_vars_40 = 1.0; /* generalized: number_of_variables <= 40 ? 1 : number_of_variables / 40.0; */
  const size_t vars_40 = 1;
  size_t i = 0;
  double result = 0.0;
  double shifted_x;

  for (i = vars_40; i < number_of_variables; ++i) {
    shifted_x = x[i] - opt[i];
    result += shifted_x * shifted_x;
  }
  result = alpha * sqrt(result / d_vars_40);
  for (i = 0; i < vars_40; ++i) {
    shifted_x = x[i] - opt[i];
    result += shifted_x * shifted_x / d_vars_40;
  }

  return result;
}

static double f_different_powers_raw(const double *x, const size_t number_of_variables, const double *opt) {
  size_t i;
  double sum = 0.0;
  double result;
  double shifted_x;

  for (i = 0; i < number_of_variables; ++i) {
    double exponent = 2.0 + (4.0 * (double)(long)i) / ((double)(long)number_of_variables - 1.0);
    shifted_x = x[i] - opt[i];
    sum += pow(fabs(shifted_x), exponent);
  }
  result = sqrt(sum);

  return result;
}


// static double f_bueche_rastrigin_raw(const double *x, const size_t number_of_variables) {

//   double tmp = 0., tmp2 = 0.;
//   size_t i;
//   double result;


//   for (i = 0; i < number_of_variables; ++i) {
//     tmp += cos(2 * coco_pi * x[i]);
//     tmp2 += x[i] * x[i];
//   }
//   result = 10.0 * ((double) (long) number_of_variables - tmp) + tmp2 + 0;
//   return result;
// }

// static double f_linear_slope_raw(const double *x,
//                                  const size_t number_of_variables,
//                                  const double *best_parameter) {

//   static const double alpha = 100.0;
//   size_t i;
//   double result = 0.0;
  
//   if (coco_vector_contains_nan(x, number_of_variables))
//     return NAN;
    
//   for (i = 0; i < number_of_variables; ++i) {
//     double base, exponent, si;

//     base = sqrt(alpha);
//     exponent = (double) (long) i / ((double) (long) number_of_variables - 1);
//     if (best_parameter[i] > 0.0) {
//       si = pow(base, exponent);
//     } else {
//       si = -pow(base, exponent);
//     }
//     /* boundary handling */
//     if (x[i] * best_parameter[i] < 25.0) {
//       result += 5.0 * fabs(si) - si * x[i];
//     } else {
//       result += 5.0 * fabs(si) - si * best_parameter[i];
//     }
//   }

//   return result;
// }

// static double f_attractive_sector_raw(const double *x,
//                                       const size_t number_of_variables,
//                                       const double *opt) {
//   size_t i;
//   double result;
//   double shifted_x;
//   result = 0.0;
//   for (i = 0; i < number_of_variables; ++i) {
//     shifted_x = x[i] - opt[i];
//     if (opt[i] * shifted_x > 0.0) {
//       result += 100.0 * 100.0 * shifted_x * shifted_x;
//     } else {
//       result += shifted_x * shifted_x;
//     }
//   }
//   return result;
// }

// static double f_step_ellipsoid_raw(const double *x, const size_t number_of_variables, f_step_ellipsoid_data_t *data) {
  
//   static const double condition = 100;
//   static const double alpha = 10.0;
//   size_t i, j;
//   double penalty = 0.0, x1;
//   double result;

//   for (i = 0; i < number_of_variables; ++i) {
//     double tmp;
//     tmp = fabs(x[i]) - 5.0;
//     if (tmp > 0.0)
//       penalty += tmp * tmp;
//   }
  
//   for (i = 0; i < number_of_variables; ++i) {
//     double c1;
//     data->x[i] = 0.0;
//     c1 = sqrt(pow(condition / 10., (double) i / (double) (number_of_variables - 1)));
//     for (j = 0; j < number_of_variables; ++j) {
//       data->x[i] += c1 * data->rot2[i][j] * (x[j] - data->xopt[j]);
//     }
//   }
//   x1 = data->x[0];
  
//   for (i = 0; i < number_of_variables; ++i) {
//     if (fabs(data->x[i]) > 0.5) /* TODO: Documentation: no fabs() in documentation */
//       data->x[i] = coco_double_round(data->x[i]);
//     else
//       data->x[i] = coco_double_round(alpha * data->x[i]) / alpha;
//   }
  
//   for (i = 0; i < number_of_variables; ++i) {
//     data->xx[i] = 0.0;
//     for (j = 0; j < number_of_variables; ++j) {
//       data->xx[i] += data->rot1[i][j] * data->x[j];
//     }
//   }
  
//   /* Computation core */
//   result = 0.0;
//   for (i = 0; i < number_of_variables; ++i) {
//     double exponent;
//     exponent = (double) (long) i / ((double) (long) number_of_variables - 1.0);
//     result += pow(condition, exponent) * data->xx[i] * data->xx[i];
//     ;
//   }
//   result = 0.1 * coco_double_max(fabs(x1) * 1.0e-4, result) + penalty + data->fopt;
  
//   return result;
// }

// static double f_discus_raw(const double *x, const size_t number_of_variables) {

//   static const double condition = 1.0e6;
//   size_t i;
//   double result;
  
//   if (coco_vector_contains_nan(x, number_of_variables))
//     return NAN;
    
//   result = condition * x[0] * x[0];
//   for (i = 1; i < number_of_variables; ++i) {
//     result += x[i] * x[i];
//   }

//   return result;
// }

// static double f_weierstrass_raw(const double *x, const size_t number_of_variables, f_weierstrass_data_t *data) {

//   size_t i, j;
//   double result;

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   result = 0.0;
//   for (i = 0; i < number_of_variables; ++i) {
//     for (j = 0; j < F_WEIERSTRASS_SUMMANDS; ++j) {
//       result += cos(2 * coco_pi * (x[i] + 0.5) * data->bk[j]) * data->ak[j];
//     }
//   }
//   result = 10.0 * pow(result / (double) (long) number_of_variables - data->f0, 3.0);

//   return result;
// }

// static double f_schaffers_raw(const double *x, const size_t number_of_variables) {

//   size_t i = 0;
//   double result;

//   assert(number_of_variables > 1);

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   /* Computation core */
//   result = 0.0;
//   for (i = 0; i < number_of_variables - 1; ++i) {
//     const double tmp = x[i] * x[i] + x[i + 1] * x[i + 1];
//     if (coco_is_inf(tmp) && coco_is_nan(sin(50.0 * pow(tmp, 0.1))))  /* sin(inf) -> nan */
//       /* the second condition is necessary to pass the integration tests under Windows and Linux */
//       return tmp;
//     result += pow(tmp, 0.25) * (1.0 + pow(sin(50.0 * pow(tmp, 0.1)), 2.0));
//   }
//   result = pow(result / ((double) (long) number_of_variables - 1.0), 2.0);

//   return result;
// }

// static double f_griewank_rosenbrock_raw(const double *x, const size_t number_of_variables) {

//   size_t i = 0;
//   double tmp = 0;
//   double result;

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   /* Computation core */
//   result = 0.0;
//   for (i = 0; i < number_of_variables - 1; ++i) {
//     const double c1 = x[i] * x[i] - x[i + 1];
//     const double c2 = 1.0 - x[i];
//     tmp = 100.0 * c1 * c1 + c2 * c2;
//     result += tmp / 4000. - cos(tmp);
//   }
//   result = 10. + 10. * result / (double) (number_of_variables - 1);

//   return result;
// }

// static double f_schwefel_raw(const double *x, const size_t number_of_variables) {

//   size_t i = 0;
//   double result;
//   double penalty, sum;

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   /* Boundary handling*/
//   penalty = 0.0;
//   for (i = 0; i < number_of_variables; ++i) {
//     const double tmp = fabs(x[i]) - 500.0;
//     if (tmp > 0.0)
//       penalty += tmp * tmp;
//   }

//   /* Computation core */
//   sum = 0.0;
//   for (i = 0; i < number_of_variables; ++i) {
//     sum += x[i] * sin(sqrt(fabs(x[i])));
//   }
//   result = 0.01 * (penalty + 418.9828872724339 - sum / (double) number_of_variables);

//   return result;
// }

// static double f_gallagher_raw(const double *x, const size_t number_of_variables, f_gallagher_data_t *data) {
//   size_t i, j; /* Loop over dim */
//   double *tmx;
//   double a = 0.1;
//   double tmp2, f = 0., f_add, tmp, f_pen = 0., f_true = 0.;
//   double fac;
//   double result;

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   fac = -0.5 / (double) number_of_variables;

//   /* Boundary handling */
//   for (i = 0; i < number_of_variables; ++i) {
//     tmp = fabs(x[i]) - 5.;
//     if (tmp > 0.) {
//       f_pen += tmp * tmp;
//     }
//   }
//   f_add = f_pen;
//   /* Transformation in search space */
//   /* TODO: this should rather be done in f_gallagher */
//   tmx = coco_allocate_vector(number_of_variables);
//   for (i = 0; i < number_of_variables; i++) {
//     tmx[i] = 0;
//     for (j = 0; j < number_of_variables; ++j) {
//       tmx[i] += data->rotation[i][j] * x[j];
//     }
//   }
//   /* Computation core*/
//   for (i = 0; i < data->number_of_peaks; ++i) {
//     tmp2 = 0.;
//     for (j = 0; j < number_of_variables; ++j) {
//       tmp = (tmx[j] - data->x_local[j][i]);
//       tmp2 += data->arr_scales[i][j] * tmp * tmp;
//     }
//     tmp2 = data->peak_values[i] * exp(fac * tmp2);
//     f = coco_double_max(f, tmp2);
//   }

//   f = 10. - f;
//   if (f > 0) {
//     f_true = log(f) / a;
//     f_true = pow(exp(f_true + 0.49 * (sin(f_true) + sin(0.79 * f_true))), a);
//   } else if (f < 0) {
//     f_true = log(-f) / a;
//     f_true = -pow(exp(f_true + 0.49 * (sin(0.55 * f_true) + sin(0.31 * f_true))), a);
//   } else
//     f_true = f;

//   f_true *= f_true;
//   f_true += f_add;
//   result = f_true;
//   coco_free_memory(tmx);
//   return result;
// }

// static double f_katsuura_raw(const double *x, const size_t number_of_variables) {

//   size_t i, j;
//   double tmp, tmp2;
//   double result;

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   /* Computation core */
//   result = 1.0;
//   for (i = 0; i < number_of_variables; ++i) {
//     tmp = 0;
//     for (j = 1; j < 33; ++j) {
//       tmp2 = pow(2., (double) j);
//       tmp += fabs(tmp2 * x[i] - coco_double_round(tmp2 * x[i])) / tmp2;
//     }
//     tmp = 1.0 + ((double) (long) i + 1) * tmp;
//     /*result *= tmp;*/ /* Wassim TODO: delete once consistency check passed*/
//     result *= pow(tmp, 10. / pow((double) number_of_variables, 1.2));
//   }
//   /*result = 10. / ((double) number_of_variables) / ((double) number_of_variables)
//       * (-1. + pow(result, 10. / pow((double) number_of_variables, 1.2)));*/
//   result = 10. / ((double) number_of_variables) / ((double) number_of_variables)
//   * (-1. + result);

//   return result;
// }

// static double f_lunacek_bi_rastrigin_raw(const double *x,
//                                          const size_t number_of_variables,
//                                          f_lunacek_bi_rastrigin_data_t *data) {
//   double result;
//   static const double condition = 100.;
//   size_t i, j;
//   double penalty = 0.0;
//   static const double mu0 = 2.5;
//   static const double d = 1.;
//   const double s = 1. - 0.5 / (sqrt((double) (number_of_variables + 20)) - 4.1);
//   const double mu1 = -sqrt((mu0 * mu0 - d) / s);
//   double *tmpvect, sum1 = 0., sum2 = 0., sum3 = 0.;

//   assert(number_of_variables > 1);

//   if (coco_vector_contains_nan(x, number_of_variables))
//   	return NAN;

//   for (i = 0; i < number_of_variables; ++i) {
//     double tmp;
//     tmp = fabs(x[i]) - 5.0;
//     if (tmp > 0.0)
//       penalty += tmp * tmp;
//   }

//   /* x_hat */
//   for (i = 0; i < number_of_variables; ++i) {
//     data->x_hat[i] = 2. * x[i];
//     if (data->xopt[i] < 0.) {
//       data->x_hat[i] *= -1.;
//     }
//   }

//   tmpvect = coco_allocate_vector(number_of_variables);
//   /* affine transformation */
//   for (i = 0; i < number_of_variables; ++i) {
//     double c1;
//     tmpvect[i] = 0.0;
//     c1 = pow(sqrt(condition), ((double) i) / (double) (number_of_variables - 1));
//     for (j = 0; j < number_of_variables; ++j) {
//       tmpvect[i] += c1 * data->rot2[i][j] * (data->x_hat[j] - mu0);
//     }
//   }
//   for (i = 0; i < number_of_variables; ++i) {
//     data->z[i] = 0;
//     for (j = 0; j < number_of_variables; ++j) {
//       data->z[i] += data->rot1[i][j] * tmpvect[j];
//     }
//   }
//   /* Computation core */
//   for (i = 0; i < number_of_variables; ++i) {
//     sum1 += (data->x_hat[i] - mu0) * (data->x_hat[i] - mu0);
//     sum2 += (data->x_hat[i] - mu1) * (data->x_hat[i] - mu1);
//     sum3 += cos(2 * coco_pi * data->z[i]);
//   }
//   result = coco_double_min(sum1, d * (double) number_of_variables + s * sum2)
//       + 10. * ((double) number_of_variables - sum3) + 1e4 * penalty;
//   coco_free_memory(tmpvect);

//   return result;
// }

void my_evaluate_func(const double *x, double *y, const char * function_name, size_t dimension, double * optimal) {
  if(strcmp(function_name, "f1") == 0){
    //f1(x, y, dimension, optimal);
    y[0] = f_sphere_raw(x, dimension, optimal) * 1;
  }
  else if(strcmp(function_name, "f2") == 0){
    //f8(x, y, dimension, optimal);
    y[0] = f_ellipsoid_raw(x, dimension, optimal) * pow(10, -3);
  }
  else if(strcmp(function_name, "f3") == 0){
    //f3(x, y, dimension, optimal);
    y[0] = f_rastrigin_raw(x, dimension, optimal) * 0.1;
  }
  else if(strcmp(function_name, "f8") == 0){
    //f8(x, y, dimension, optimal);
    y[0] = f_rosenbrock_raw(x, dimension, optimal) * pow(10, -2);
  }
  else if(strcmp(function_name, "f12") == 0){
    //f8(x, y, dimension, optimal);
    y[0] = f_bent_cigar_raw(x, dimension, optimal) * pow(10, -4);
  }
  else if(strcmp(function_name, "f13") == 0){
    //f8(x, y, dimension, optimal);
    y[0] = f_sharp_ridge_raw(x, dimension, optimal) * 0.1;
  }
  else if(strcmp(function_name, "f14") == 0){
    //f8(x, y, dimension, optimal);
    y[0] = f_different_powers_raw(x, dimension, optimal) * 1;
  }
}