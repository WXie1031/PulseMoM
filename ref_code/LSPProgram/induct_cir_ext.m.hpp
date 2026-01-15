// Automatically translated using Matlab2cpp 0.5 on 2016-02-25 13:07:40
            
#include <armadillo>
using namespace arma ;

vec induct_cir_ext(vec re, vec len)
{
  vec Lext ;
  Lext = 2e-7*len*(log(len/re+sqrt(pow((len/re), 2)+1))-sqrt(1+pow((re/len), 2))+re/len) ;
  return Lext ;
}