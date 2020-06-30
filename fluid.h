#ifndef FLUID_H
#define FLUID_H

#include <string.h>
#include <alloca.h>

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 1, __VA_ARGS__); }

#ifndef strdupa
# define strdupa(s)                         \
    ({                                      \
      const char *old = (s);                \
      size_t len = strlen (old) + 1;        \
      char *newstr = (char *) alloca (len); \
      (char *) memcpy (newstr, old, len);   \
    })
#endif

#endif /* FLUID_H */
