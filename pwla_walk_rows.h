#include "pwla.h"

typedef struct row_getter RGETTER;

typedef ARRAY_ELEMENT* (*pwla_next_row)(RGETTER *rg);

ARRAY_ELEMENT* direct_getter(RGETTER *rg);
ARRAY_ELEMENT* keyed_getter(RGETTER *rg);

bool init_direct_getter(RGETTER *rg,
                        AHEAD *table,
                        int starting_ndx,
                        int count);

bool init_keyed_getter(RGETTER *rg,
                       AHEAD *table,
                       AHEAD *key,
                       int starting_ndx,
                       int count);

struct row_getter {
   pwla_next_row  getter;
   ARRAY_ELEMENT  *cur_el;
   ARRAY_ELEMENT  *term_el;
   void           *data;
};


