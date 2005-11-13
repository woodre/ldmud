#ifndef __EFUNS_H__
#define __EFUNS_H__ 1

#include "driver.h"
#include "datatypes.h"

/* --- Prototypes --- */

extern struct svalue *f_make_shared_string(struct svalue *);
extern struct svalue *f_upper_case(struct svalue *);

extern struct svalue *x_all_environment(struct svalue *, int);
extern struct svalue *f_object_info (struct svalue *sp);

#endif /* __EFUNS_H__ */

