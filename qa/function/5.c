/* $HopeName$
TEST_HEADER
 summary = (regression test) have an ambiguous reference to a reserved but not committed object, and then allocate lots more with another AP, to make it collect
 language = c
 link = testlib.o newfmt.o
END_HEADER
*/

#include "testlib.h"
#include "mpscamc.h"
#include "newfmt.h"

#define OBJ_SIZE (MPS_PF_ALIGN*32)

void *stackpointer;

static void test(void)
{
 mps_space_t space;
 mps_pool_t pool;
 mps_thr_t thread;
 mps_root_t root;

 mps_fmt_t format;
 mps_ap_t apA;
 mps_ap_t apB;

 mps_addr_t p;
 mycell *q;

 int i;

 cdie(mps_space_create(&space), "create space");

 cdie(mps_thread_reg(&thread, space), "register thread");

 cdie(
  mps_root_create_reg(&root, space, MPS_RANK_AMBIG, 0, thread,
   mps_stack_scan_ambig, stackpointer, 0),
  "create root");

 cdie(
  mps_fmt_create_A(&format, space, &fmtA),
  "create format");

 formatcomments = 0;

 cdie(
  mps_pool_create(&pool, space, mps_class_amc(), format),
  "create pool");

 cdie(
  mps_ap_create(&apA, pool, MPS_RANK_EXACT), "create apA");

 cdie(
  mps_ap_create(&apB, pool, MPS_RANK_EXACT), "create apB");


 die(mps_reserve(&p, apA, OBJ_SIZE), "Reserve: ");

 for (i=1; i<10000; i++)
 {
  allocone(apB, 100);
 }

 q=p;
 q->data.tag = MCdata;
 q->data.id = 0;
 q->data.numrefs = 0;
 q->data.size = OBJ_SIZE;
 (void) mps_commit(apA, p, OBJ_SIZE);

 mps_ap_destroy(apA);
 comment("Destroyed apA.");

 mps_ap_destroy(apB);
 comment("Destroyed apB.");

 mps_pool_destroy(pool);
 comment("Destroyed pool.");

 mps_fmt_destroy(format);
 comment("Destroyed format.");

 mps_root_destroy(root);
 comment("Destroyed root.");

 mps_thread_dereg(thread);
 comment("Deregistered thread.");

 mps_space_destroy(space);
 comment("Destroyed space.");
}

int main(void)
{
 void *m;
 stackpointer=&m; /* hack to get stack pointer */

 easy_tramp(test);
 pass();
 return 0;
}
