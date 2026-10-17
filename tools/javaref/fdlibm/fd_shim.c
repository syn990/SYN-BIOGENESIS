/* sqrt is correctly rounded in IEEE 754 everywhere, as StrictMath.sqrt is;
 * e_pow.c only needs it for pow(x, 0.5). */
#include <math.h>
double fd_sqrt(double x) { return sqrt(x); }
