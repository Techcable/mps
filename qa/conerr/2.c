/* $HopeName$
TEST_HEADER
 summary = destroy a space which isn't a space
 language = c
 link = testlib.o
END_HEADER
*/

#include "testlib.h"

static void test(void)
{
 mps_space_t space;

 space = malloc(64);
 mps_space_destroy(space);
 comment("Destroy space.");
}

int main(void)
{
 easy_tramp(test);
 return 0;
}
