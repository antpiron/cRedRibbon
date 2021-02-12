#ifndef __RRHO_H
#define __RRHO_H

#include <stddef.h>
#include "ale.h"

/* mode */
enum {
      RRHO_HYPER = 0,
      RRHO_HYPER_TWO_TAILED,
      RRHO_HYPER_TWO_TAILED_R_MODULE
};

/* direction */
enum {
      RRHO_DOWN_DOWN = 0,
      RRHO_UP_UP,
      RRHO_DOWN_UP,
      RRHO_UP_DOWN
};

/* Algorithm */
enum {
      RRHO_CLASSIC = 0,
      RRHO_EA,
};

struct rrho
{
  size_t n;
  double *a, *b;
  size_t *index_a, *index_b;
  struct bitset bs_a;
  struct bitset bs_b;
  struct bitset bs_and;
  size_t n_a, n_b;
};

struct rrho_result
{
  long double pvalue;
  int direction;
  size_t count;
};

struct rrho_permutation_result
{
  long double pvalue;
  long double pvalue_ks;
  long double stat_ks;
};

struct rrho_coord
{
  size_t i, j;
};

int rrho_init(struct rrho *rrho, size_t n, double a[n], double b[n]);
int rrho_destroy(struct rrho *rrho);

int rrho_hyper_two_tailed_as_r_module(struct rrho *rrho, size_t i, size_t j, struct rrho_result *res);
int rrho_hyper_two_tailed(struct rrho *rrho, size_t i, size_t j, struct rrho_result *res);
int rrho_hyper(struct rrho *rrho, size_t i, size_t j, struct rrho_result *res);
static inline int rrho_generic(struct rrho *rrho, size_t i, size_t j, int mode, struct rrho_result *res);
// int rrho_permutation_generic(struct rrho *rrho, size_t i, size_t j, int mode, size_t niter, struct rrho_result *res);

void rrho_intersect(struct rrho *rrho, size_t i, size_t j, int directions, struct bitset *bs_res);

int rrho_rectangle(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
		   size_t m, size_t n, int mode, int log_flag, double dst[m][n]);

int rrho_rectangle_min(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
		        size_t m, size_t n, int mode, int direction, struct rrho_coord *coord);
int rrho_rectangle_min_ea(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
			  int mode, int direction, struct rrho_coord *coord);
static inline int rrho_rectangle_min_generic(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
					     size_t m, size_t n, int mode, int direction, int algorithm, struct rrho_coord *coord);

int rrho_permutation_generic(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
			     size_t m, size_t n, int mode, int direction, int algorithm,
			     size_t niter, long double pvalue, struct rrho_permutation_result *res);
// inline

static inline int
rrho_generic(struct rrho *rrho, size_t i, size_t j, int mode, struct rrho_result *res)
{
  if (RRHO_HYPER_TWO_TAILED == mode)
    return rrho_hyper_two_tailed(rrho, i, j, res);

  if (RRHO_HYPER_TWO_TAILED_R_MODULE == mode)
    return rrho_hyper_two_tailed_as_r_module(rrho, i, j, res);

  return rrho_hyper(rrho, i, j, res);
}

static inline int
rrho_rectangle_min_generic(struct rrho *rrho, size_t i, size_t j, size_t ilen, size_t jlen,
			   size_t m, size_t n, int mode, int direction, int algorithm, struct rrho_coord *coord)
{
  if (RRHO_CLASSIC == algorithm)
    return rrho_rectangle_min(rrho, i, j, ilen, jlen, m, n, mode, direction, coord);

  return rrho_rectangle_min_ea(rrho, i, j, ilen, jlen, mode, direction, coord);
}



#endif
