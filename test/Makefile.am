LDADD = $(top_srcdir)/src/libredribbon.la
AM_LDFLAGS = -static
AM_CFLAGS = $(OPENMP_CFLAGS) -g -O0 -fsanitize=bounds -I$(top_srcdir)/src

check_PROGRAMS = t-rrho t-rrho-hyper t-rrho-generic t-rrho-permutation-generic t-rrho-permutation-generic-shuffle t-rrho-intersect t-rrho-rectangle  t-rrho-rectangle-min t-rrho-rectangle-min-ea \
	t-expression-prediction t-ldfit-prediction

TESTS = $(check_PROGRAMS)

check-valgrind:	$(check_PROGRAMS)
	@for prog in $(check_PROGRAMS); do echo "valgrind $$prog"; valgrind -q ./$$prog; done
