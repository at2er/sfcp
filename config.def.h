#define TERM getenv("TERM")

/* wrap script
 * $1 string: mode(open, save).
 * $2 string: output.
 * $3 bool: multiple.
 *
 * In open mode:
 *   $4 bool: directory only.
 *   $5 string?: default path.
 *
 * In save mode:
 *   $4 string?: default name.
 */
static const char wrap[] = "wrap-sfcp-fm";

#if defined (__linux__)
#include <linux/limits.h>
#else
#define PATH_MAX 4096
#endif
