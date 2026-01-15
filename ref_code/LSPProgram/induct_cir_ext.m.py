# encoding: utf-8
#
# Supplement file
#
# Valid inputs:
#
# uword   int     float   double cx_double
# uvec    ivec    fvec    vec    cx_vec
# urowvec irowvec frowvec rowvec cx_rowvec
# umat    imat    fmat    mat    cx_mat
# ucube   icube   fcube   cube   cx_cube
#
# char    string  struct  structs func_lambda

functions = {
  "induct_cir_ext" : {
    "Lext" : "vec",
     "len" : "vec",
      "re" : "vec",
  },
}
includes = [
  '#include <armadillo>',
  'using namespace arma ;',
]