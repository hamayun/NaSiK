#include <reent.h>

struct _reent _impure_data = _REENT_INIT(_impure_data);
struct _reent * _impure_ptr = &_impure_data;
struct _reent *_CONST _global_impure_ptr = &_impure_data;
