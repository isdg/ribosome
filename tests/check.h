/* tests/check.h - the assertion harness shared by every test binary.
 *
 * Deliberately tiny. The sweeps here run hundreds of thousands of cases,
 * so the only reporting that helps is "the first few divergences, then
 * stop talking" - not one record per assertion.
 */

#ifndef RIBOSOME_TEST_CHECK_H
#define RIBOSOME_TEST_CHECK_H

#include <stdio.h>

#define CHECK_MAX_REPORT 10

static long checks = 0;
static long failures = 0;

#define CHECK(cond, ...)                                  \
  do {                                                    \
    checks++;                                             \
    if (!(cond)) {                                        \
      failures++;                                         \
      if (failures <= CHECK_MAX_REPORT) {                 \
        printf("  FAIL  ");                               \
        printf(__VA_ARGS__);                              \
        printf("\n");                                     \
      } else if (failures == CHECK_MAX_REPORT + 1) {      \
        printf("  ... further failures muted\n");         \
      }                                                   \
    }                                                     \
  } while (0)

static int check_summary(const char *name) {
  printf("%s: %ld checks, %ld failures\n", name, checks, failures);
  return failures != 0;
}

#endif /* RIBOSOME_TEST_CHECK_H */
