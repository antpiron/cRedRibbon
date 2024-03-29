#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

#include "cRedRibbon.h"
#include <ale.h>
#include <ale/ea.h>
#include <ale/sort.h>


int
rrho_init(struct rrho *rrho, size_t n, double a[n], double b[n])
{
  rrho->n = n;
  rrho->a = a;
  rrho->b = b;
  rrho->n_a = rrho->n_b = 0;

  rrho->index_a = malloc(sizeof(size_t) * n);
  rrho->index_b = malloc(sizeof(size_t) * n);

  sort_q_indirect(rrho->index_a, rrho->a, n, sizeof(double), sort_compar_double, NULL);
  sort_q_indirect(rrho->index_b, rrho->b, n, sizeof(double), sort_compar_double, NULL);

  bitset_init(&rrho->bs_a,n);
  bitset_init(&rrho->bs_b, n);
  bitset_init(&rrho->bs_and, n);
 
  return 0;
}

int
rrho_destroy(struct rrho *rrho)
{

  free(rrho->index_a);
  free(rrho->index_b);

  bitset_destroy(&rrho->bs_a);
  bitset_destroy(&rrho->bs_b);
  bitset_destroy(&rrho->bs_and);
 
  return 0;
}

static void
update_bitsets(struct rrho *rrho, size_t i, size_t j)
{
  // TODO: improve performance
  
  // bitset_reset(&rrho->bs_a);
  // bitset_reset(&rrho->bs_b);
  // bitset_reset(&rrho->bs_and);

  if (rrho->n_a < i+1)
    {
      for (size_t ii = rrho->n_a ; ii < i+1 ; ii++)
	bitset_set(&rrho->bs_a,  rrho->index_a[ii]);
    }
  else
    {
      for (size_t ii = i+1 ; ii < rrho->n_a ; ii++)
	bitset_unset(&rrho->bs_a,  rrho->index_a[ii]);
    }

  if (rrho->n_b < j+1)
    {
      for (size_t jj = rrho->n_b ; jj < j+1 ; jj++)
	bitset_set(&rrho->bs_b,  rrho->index_b[jj]);
    }
  else
    {
      for (size_t jj = j+1 ; jj < rrho->n_b ; jj++)
	bitset_unset(&rrho->bs_b,  rrho->index_b[jj]);	    
    }
   
  // bitset_and(&rrho->bs_and, &rrho->bs_a, &rrho->bs_b);

  rrho->n_a = i+1;
  rrho->n_b = j+1;
}

static size_t
count_intersect(struct rrho *rrho, size_t i, size_t j)
{
  size_t count = 0;
  
  update_bitsets(rrho, i, j);
  bitset_and(&rrho->bs_and, &rrho->bs_a, &rrho->bs_b);

  count = bitset_ones(&rrho->bs_and);
  
  return count;
}

void
rrho_intersect(struct rrho *rrho, size_t i, size_t j, int directions, struct bitset *bs_res)
{
  struct bitset not_a, not_b;

  bitset_reset(bs_res);
  
  if (RRHO_DOWN_DOWN == directions)
    {
      update_bitsets(rrho, i, j);
      bitset_and(bs_res, &rrho->bs_a, &rrho->bs_b);
    }
  else if (RRHO_UP_UP == directions)
    {
      update_bitsets(rrho, i-1, j-1);
      
      bitset_init(&not_a, rrho->n);
      bitset_init(&not_b, rrho->n);

      bitset_not(&not_a, &rrho->bs_a);
      bitset_not(&not_b, &rrho->bs_b);
      bitset_and(bs_res, &not_a, &not_b);

      bitset_destroy(&not_a);
      bitset_destroy(&not_b);
    }
  else if (RRHO_DOWN_UP == directions)
    {
      update_bitsets(rrho, i, j-1);

      bitset_init(&not_b, rrho->n);
       
      bitset_not(&not_b, &rrho->bs_b);
      bitset_and(bs_res, &rrho->bs_a, &not_b);

      bitset_destroy(&not_b);
    }
  else if (RRHO_UP_DOWN == directions)
    {
      update_bitsets(rrho, i-1, j);

      bitset_init(&not_a, rrho->n);
      
      bitset_not(&not_a, &rrho->bs_a); 
      bitset_and(bs_res, &not_a, &rrho->bs_b);

      bitset_destroy(&not_a);
    }
}


int
rrho_hyper_two_tailed_as_r_module(struct rrho *rrho, size_t i, size_t j, struct rrho_result *res)
{
  // try http://showmethevotes.org/2016/12/11/hypergeometric-p-value-computation-for-exit-poll-results-using-excel/
  //    2*min(stats_hyper_F(count, i+1, j+1, rrho->n), 1.0 - stats_hyper_F(count-1, i+1, j+1, rrho->n),  0.5)
  // double stats_hyper_F(long k, long K, long n, long N)
  size_t count = count_intersect(rrho, i, j);
  long double mean = (double) (i+1) * (double) (j+1) / rrho->n;
  long double symmetric = 2*mean - count;
  long lower = (count < symmetric)?count:ceil(symmetric);
  long upper = (count < symmetric)?floor(symmetric):count;

  if ( (double) count <= mean )
    res->direction = -1;
  else
    res->direction = 1;
  
  res->pvalue = stats_hyper_Fl(lower, i+1, j+1, rrho->n) +
    stats_hyper_taill(upper, i+1, j+1, rrho->n, STATS_UPPER);
  // res->fdr = (0 == count)?-1:mean / count;
  
  if (res->pvalue > 1)
    res->pvalue = 1;

  res->count = count;
  
  return 0;
}

int
rrho_hyper_two_tailed(struct rrho *rrho, size_t i, size_t j, struct rrho_result *res)
{
  // http://showmethevotes.org/2016/12/11/hypergeometric-p-value-computation-for-exit-poll-results-using-excel/
  //    2*min(stats_hyper_F(count, i+1, j+1, rrho->n), 1.0 - stats_hyper_F(count-1, i+1, j+1, rrho->n),  0.5)
  // double stats_hyper_F(long k, long K, long n, long N)
  size_t count = count_intersect(rrho, i, j);
  long double mean = (double) (i+1) * (double) (j+1) / rrho->n;
  long double min_pval = 0.5;
  long double pval;
  

  if ( (double) count <= mean )
    res->direction = -1;
  else
    res->direction = 1;

  pval = stats_hyper_Fl(count, i+1, j+1, rrho->n);
  if ( pval < min_pval )
    min_pval = pval;

  pval = stats_hyper_taill(count, i+1, j+1, rrho->n, STATS_UPPER);
  if  ( pval < min_pval )
    min_pval = pval;
  
  res->pvalue = 2 * min_pval;
  
  if (res->pvalue > 1)
    res->pvalue = 1;

  // res->fdr = (0 == count)?-1:mean / count;
  res->count = count;
  
  return 0;
}

int
rrho_hyper(struct rrho *rrho, size_t i, size_t j, struct rrho_result *res)
{
  // https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2943622/
  // double stats_hyper_F(long k, long K, long n, long N)
  size_t count = count_intersect(rrho, i, j);

  res->pvalue = stats_hyper_taill(count, i+1, j+1, rrho->n, STATS_UPPER);
  res->direction = 1;

  if (res->pvalue > 1)
    res->pvalue = 1;
  // res->fdr = (0 == count)?-1:mean / count;
  res->count = count;
  
  return 0;
}


static long double
mylogl(long double x)
{
  long double res;
  
  res = logl(x);
  
  if ( isinf(res) )
    res = logl(LDBL_MIN);
  
  // LDBL_MIN == 3.36210314311209350626e-4932 (on x86); log = -4928.485
  if ( isinf(res) )
    res = 4928.485;

  return res;
}

int
rrho_rectangle(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
	       struct rrho_rectangle_params *params, int mode, int log_flag, double dst[params->m][params->n])
{
  struct rrho_result res;
  double istep = (double) ilen / (double) params->m;
  double jstep = (double) jlen / (double) params->n;
  long double pvalue;      
  
  for (size_t y = 0 ; y < params->m ; y++)
    {
      for (size_t x = 0 ; x < params->n ; x++)
	{
	  size_t ii = round(i + y * istep);
	  size_t jj = round(j + x * jstep);

	  /* Let be safe */
	  if (ii >= rrho->n)
	    ii = rrho->n - 1;
	  
	  if (jj >= rrho->n)
	    jj = rrho->n - 1;
	  
	  rrho_generic(rrho, ii, jj, mode, &res);

	  pvalue = res.pvalue;
	  if (log_flag)
	    {
	      pvalue = -mylogl(pvalue);
	    }
	  dst[y][x] = copysignl(pvalue, res.direction);
	}
    }
  
  return 0;
}

int
rrho_rectangle_min(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
		    struct rrho_rectangle_params *params, int mode, int direction, struct rrho_coord *coord)
{
  struct rrho_result res;
  double istep = (double) ilen / (double) params->m;
  double jstep = (double) jlen / (double) params->n;
  long double pvalue = 1.1;
  int ret = -1;

  for (size_t y = 0 ; y < params->m ; y++)
    {
      for (size_t x = 0 ; x < params->n ; x++)
	{
	  size_t ii = round(i + y * istep);
	  size_t jj = round(j + x * jstep);

	  /* Let be safe */
	  if (ii >= rrho->n)
	    ii = rrho->n - 1;
	  
	  if (jj >= rrho->n)
	    jj = rrho->n - 1;

	  rrho_generic(rrho, ii, jj, mode, &res);

	  if ( copysign(1, res.direction) == copysign(1, direction) && res.pvalue < pvalue )
	    {
	      pvalue = res.pvalue;
	      coord->i = ii;
	      coord->j = jj;
	      ret = 0;
	    }
	}
    }
  
  return ret;
}

static inline
int
stop_condition(size_t iter, size_t below)
{
  if (iter < 32 || below < 5)
    return 0;

  long double  pvalue_perm = (long double) below / (long double) iter;
  if ( pvalue_perm <= 1.0L / sqrtl(iter) )
    return 0;
  
  return 1;
}

struct beta_params
{
  long double alpha, beta;
};

static long double
beta_cdfl(long double x, void *cls)
{
  struct beta_params *params = cls;
  return stats_beta_Fl(x, params->alpha, params->beta);
}

int
rrho_permutation_generic(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
			 void *params, struct stats_permutation *permutation, int mode, int direction, int algorithm,
			 size_t niter, long double pvalue, struct rrho_permutation_result *res_perm)
{
  long double *pvalues = malloc(sizeof(long double) * niter);
  long double threshold;
  const long double alpha = 0.05;
  const long double alpha_ks = 0.05;
  struct beta_params bparams;

  /* printf("b = "); */
  /* for (size_t i = 0 ; i <  (rrho->n < 10?rrho->n:10) ; i++) */
  /*   printf("%f\t", rrho->b[i]); */
  /* printf("\n"); */

#pragma omp parallel
  {
    size_t sizeb = sizeof(double) * rrho->n;
    double *b = malloc(sizeb);
    // memcpy(b, rrho->b, sizeb);
    
#pragma omp for
    for (size_t iter = 0 ; iter < niter ; iter++)
      {
	struct rrho rrho_perm;
	struct rrho_coord coord;
	struct rrho_result res;
	void *params_ptr = params;
	int ret;
	
	// stats_shuffle(b, rrho->n, sizeof(double));
	stats_permutation(permutation, b);
	/* printf("ITER %zd:\n", iter); */
	/* for (size_t i = 0 ; i <  (rrho->n < 10?rrho->n:10) ; i++) */
	/*   printf("%f\t", rrho->a[i]); */
	/* printf("\n"); */
	/* for (size_t i = 0 ; i <  (rrho->n < 10?rrho->n:rrho->n) ; i++) */
	/*   printf("%f\t", b[i]); */
	/* printf("\n"); */
	
	rrho_init(&rrho_perm, rrho->n, rrho->a, b);

	ret = rrho_rectangle_min_generic(&rrho_perm, i, j, ilen, jlen, params_ptr, mode, direction, algorithm, &coord);
	if (ret < 0)
	  {
	    // printf("rrho_rectangle_min_generic returned -1\n");
	    res.pvalue = 1;
	  }
	else
	  {
	    // printf("(%zu, %zu)\n", coord.i, coord.j);
	    rrho_generic(&rrho_perm, coord.i, coord.j, mode, &res);
	  }
	
	pvalues[iter] = res.pvalue;
	// printf("pvalues %Le\n", res.pvalue);
	
	rrho_destroy(&rrho_perm);
      }
    free(b);
  }

  /* printf("End of iter\n"); */
  /* for (size_t i = 0 ; i < niter ; i++) */
  /*   printf("%Le \t", pvalues[i]); */
  /* printf("\n"); */

  stats_beta_fitl(niter, pvalues, &bparams.alpha, &bparams.beta);
  // printf("End of stats_beta_fitl\n");
  stats_ks_testl(niter, pvalues, beta_cdfl, &bparams, &res_perm->pvalue_ks, &res_perm->stat_ks);
  // printf("End of stats_ks_testl\n");

  if (res_perm->pvalue_ks > alpha_ks)
    {
      threshold = stats_beta_F_invl(alpha, bparams.alpha, bparams.beta);
      // printf("End of stats_beta_F_inv\n");
    }
  else
    {
      struct stats_ecdfl ecdf;

      stats_ecdf_initl(&ecdf, niter, pvalues);

      threshold = stats_ecdf_F_invl(&ecdf, alpha);
      if (threshold < 0)
	threshold = 0;

      stats_ecdf_destroyl(&ecdf);
    }
  
  res_perm->pvalue = pvalue * alpha / threshold;
  // printf("p-value = %Le,  alpha = %Le, threshold = %Le, res_perm->pvalue = %Le\n", pvalue, alpha, threshold, res_perm->pvalue);

  /* sometimes threshold returned by ecdf estimation is 0 making the padj infinite. Limit to bonferroni correction in this case.*/
  if (isinf(res_perm->pvalue))
    res_perm->pvalue = pvalue * rrho->n;

  if (res_perm->pvalue > 1)
    res_perm->pvalue = 1;
  
  free(pvalues);

  // printf("End of rrho_permutation_generic\n");

  return 0;
}

static double
fitness(struct rrho_coord x,  struct rrho_rectangle_params_ea *params)
{
  struct rrho_result res;
  long double ret;
  
  rrho_generic(params->rrho, x.i, x.j, params->mode, &res);
  if ( copysign(1, res.direction) != copysign(1, params->direction) )
    return 0;

  ret = -logl(res.pvalue);
  // if ( isinf(ret) )
  //  ret = -log(LDBL_EPSILON);
  // printf("(%zu,%zu) = %Le, log = %Le, count = %zu, direction = %d\n", x.i, x.j, res.pvalue, ret, res.count, res.direction);
  return (ret < 0)?0:ret;
}

static inline size_t
in_range(size_t x, size_t i, size_t ilen)
{
  if ( x < i )
    return i;
  if (x >= i + ilen )
    return i + ilen - 1;
    
  return x;
}

static void
mutate(struct rrho_coord *x, struct rrho_rectangle_params_ea *params)
{
  if (stats_unif_std_rand() <= params->prob_mutation)
    {
      double sigma = (stats_unif_std_rand() <= 0.5)? params->sigma : params->ilen;
      double rand = stats_norm_rand(x->i, sigma);
      size_t i = round(rand);
      x->i = in_range(i, params->i, params->ilen);
    }
  if (stats_unif_std_rand() <= params->prob_mutation)
    {
      double sigma = (stats_unif_std_rand() <= 0.5)? params->sigma : params->jlen;
      double rand = stats_norm_rand(x->j, sigma);
      size_t j = round(rand);
      x->j = in_range(j, params->j, params->jlen);
    }
}


static void
mate(struct rrho_coord *x, struct rrho_coord m1, struct rrho_coord m2, struct rrho_rectangle_params_ea *params)
{
  size_t i = round( m1.i + stats_unif_std_rand() * ( (double) m2.i - (double) m1.i) );
  size_t j = round( m1.j + stats_unif_std_rand() * ( (double) m2.j - (double) m1.j) );
  x->i = in_range(i, params->i, params->ilen);
  x->j = in_range(j, params->j, params->jlen);
}
  
EA_INIT(optim,struct rrho_coord,mate,mutate,fitness,struct rrho_rectangle_params_ea *);

int
rrho_rectangle_min_ea(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
		      struct rrho_rectangle_params_ea *params_init, int mode, int direction, struct rrho_coord *coord)
{
  struct ea_optim ea;
  struct rrho_coord *population;
  struct rrho_rectangle_params_ea params;

  if (NULL == params_init)
    rrho_init_params_ea(rrho, &params);
  else
    params = *params_init;

  params.mode = mode;
  params.rrho = rrho;
  params.i = i;
  params.j = j;
  params.ilen = ilen;
  params.jlen = jlen;
  params.direction = direction;

  population = malloc(params.max_pop_size * sizeof(struct rrho_coord));
  
  size_t c = 0;
  if ( NULL != params.initial_population_func )
    {
      for ( ; c < params.max_pop_size ; c++)
	{
	  if ( ! params.initial_population_func(population + c, c, &params, params.initial_population_cls) )
	    break;
	}
    }
  
  for ( ; c < params.max_pop_size ; c++)
    {
      population[c].i = in_range(floor(stats_unif_rand(i, i+ilen)), params.i, params.ilen);
      population[c].j = in_range(floor(stats_unif_rand(j, j+jlen)), params.j, params.jlen);
    }
  
  ea_optim_init(&ea, params.min_pop_size, params.max_pop_size, population, &params);

  for (size_t iter = 0 ; iter < params.niter ; iter++)
    {
      ea_optim_next_generation(&ea, &params);

      size_t index_first = ea.fitness_index[0];
      size_t index_last = ea.fitness_index[params.min_pop_size - 1];

      if (population[index_first].i == population[index_last].i &&
	  population[index_first].j == population[index_last].j)
	break;
      /* printf("%3zu: ", iter); */
      /* for (size_t c = 0 ; c < ( (params.max_pop_size > 5 )?5:params.max_pop_size ) ; c++) */
      /* 	{ */
      /* 	  size_t index = ea.fitness_index[c]; */
      /* 	  printf("(%zu, %zu, %e)", population[index].i, population[index].j, ea.fitness[index]); */
      /* 	  if (c+1 < params.max_pop_size) */
      /* 	    { */
      /* 	      size_t index1 = ea.fitness_index[c+1]; */
      /* 	      if ( ea.fitness[index] > ea.fitness[index1] ) */
      /* 		printf(" < "); */
      /* 	      else if ( ea.fitness[index] == ea.fitness[index1] ) */
      /* 		printf(" == "); */
      /* 	      else */
      /* 		printf(" >= "); */
      /* 	    } */
		
      /* 	} */
      /* printf("\n"); */
    }

  size_t index0 = ea.fitness_index[0];
  *coord = ea.population[index0];

  ea_optim_destroy(&ea);
  free(population);

  return 0;
}
