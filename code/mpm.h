/* mpm.h: MEMORY POOL MANAGER DEFINITIONS
 *
 * $Id$
 * Copyright (c) 2001-2020 Ravenbrook Limited.  See end of file for license.
 * Portions copyright (C) 2002 Global Graphics Software.
 *
 * .trans.bufferinit: The Buffer data structure has an Init field and
 * an Init method, there's a name clash.  We resolve this by calling the
 * accessor BufferGetInit.
 *
 * .critical.macros: In manual-allocation-bound programs using MVFF,
 * PoolFree and the Land generic functions are on the critical path
 * via mps_free. In non-checking varieties we provide macro
 * alternatives to these functions that call the underlying methods
 * directly, giving a few percent improvement in performance.
 */

#ifndef mpm_h
#define mpm_h

#include "config.h"
#include "misc.h"
#include "check.h"

#include "event.h"
#include "lock.h"
#include "prmc.h"
#include "prot.h"
#include "sp.h"
#include "th.h"
#include "ss.h"
#include "mpslib.h"
#include "ring.h"
#include "tract.h" /* only for certain Seg macros */
#include "arg.h"
#include "mpmtypes.h"
#include "mpmst.h"


/* MPMCheck -- check MPM assumptions */

extern Bool MPMCheck(void);


/* Miscellaneous Checks -- see <code/mpm.c> */

/* <design/type#.bool.check> */
#define BoolCheck(b) ((unsigned)(b) <= 1)

extern Bool FunCheck(Fun f);
#define FUNCHECK(f)     (FunCheck((Fun)f))

extern Bool ShiftCheck(Shift shift);
extern Bool AttrCheck(Attr attr);
extern Bool RootVarCheck(RootVar rootVar);
extern Bool AccessSetCheck(AccessSet mode);


/* Address/Size Interface -- see <code/mpm.c> */

extern Bool AlignCheck(Align align);

extern Bool (WordIsAligned)(Word word, Align align);
#define WordIsAligned(w, a)     (((w) & ((a) - 1)) == 0)

extern Word (WordAlignUp)(Word word, Align align);
#define WordAlignUp(w, a)       (((w) + (a) - 1) & ~((Word)(a) - 1))

/* Rounds w up to a multiple of r, see <code/mpm.c> for exact behaviour */
extern Word (WordRoundUp)(Word word, Size round);
#define WordRoundUp(w, r)       (((w)+(r)-1) - ((w)+(r)-1)%(r))

extern Word (WordAlignDown)(Word word, Align align);
#define WordAlignDown(w, a)     ((w) & ~((Word)(a) - 1))

#define size_tAlignUp(s, a) ((size_t)WordAlignUp((Word)(s), a))

#define PointerAdd(p, s) ((void *)((char *)(p) + (s)))
#define PointerSub(p, s) ((void *)((char *)(p) - (s)))

#define PointerOffset(base, limit) \
  ((size_t)((char *)(limit) - (char *)(base)))

#define PointerAlignUp(p, s) \
  ((void *)WordAlignUp((Word)(p), (Align)(s)))

#define AddrAdd(p, s) ((Addr)PointerAdd((void *)(p), s))
#define AddrSub(p, s) ((Addr)PointerSub((void *)(p), s))

#define AddrOffset(b, l) \
  ((Size)(PointerOffset((void *)(b), (void *)(l))))

extern Addr (AddrAlignDown)(Addr addr, Align align);
#define AddrAlignDown(p, a) ((Addr)WordAlignDown((Word)(p), a))

#define AlignWord(s)            ((Word)(s))

#define AddrIsAligned(p, a)     WordIsAligned((Word)(p), a)
#define AddrAlignUp(p, a)       ((Addr)WordAlignUp((Word)(p), a))
#define AddrRoundUp(p, r)       ((Addr)WordRoundUp((Word)(p), r))

#define ReadonlyAddrAdd(p, s) ((ReadonlyAddr)((const char *)(p) + (s)))

#define SizeIsAligned(s, a)     WordIsAligned((Word)(s), a)
#define SizeAlignUp(s, a)       ((Size)WordAlignUp((Word)(s), a))
#define SizeAlignDown(s, a)     ((Size)WordAlignDown((Word)(s), a))
/* r not required to be a power of 2 */
#define SizeRoundUp(s, r)       ((Size)WordRoundUp((Word)(s), (Size)(r)))

#define IndexIsAligned(s, a)    WordIsAligned((Word)(s), a)
#define IndexAlignUp(s, a)      ((Index)WordAlignUp((Word)(s), a))
#define IndexAlignDown(s, a)    ((Index)WordAlignDown((Word)(s), a))

#define AlignIsAligned(a1, a2)  WordIsAligned((Word)(a1), a2)


extern Addr (AddrSet)(Addr target, Byte value, Size size);
/* This is one of the places that implements Addr, so it's allowed to */
/* convert to void *, see <design/type#.addr.ops.mem>. */
#define AddrSet(target, value, size) \
  mps_lib_memset(target, (int)(value), size)

extern Addr (AddrCopy)(Addr target, Addr source, Size size);
#define AddrCopy(target, source, size) \
  mps_lib_memcpy(target, source, size)

extern int (AddrComp)(Addr a, Addr b, Size size);
#define AddrComp(a, b, size) \
  mps_lib_memcmp(a, b, size)


/* ADDR_PTR -- turns an Addr into a pointer to the given type */

#define ADDR_PTR(type, addr) ((type *)(addr))


/* Clock */

#define ClockNow() ((Clock)mps_clock())
#define ClocksPerSec() ((Clock)mps_clocks_per_sec())


/* Result codes */

extern Bool ResIsAllocFailure(Res res);


/* Logs and Powers
 *
 * SizeIsP2 returns TRUE if and only if size is a non-negative integer
 * power of 2, and FALSE otherwise.
 *
 * SizeLog2 returns the logarithm in base 2 of size.  size must be a
 * power of 2.
 *
 * SizeFloorLog2 returns the floor of the logarithm in base 2 of size.
 * size can be any positive non-zero value.  */

extern Bool (SizeIsP2)(Size size);
#define SizeIsP2(size) WordIsP2((Word)size)
extern Shift SizeLog2(Size size);
extern Shift SizeFloorLog2(Size size);

extern Bool (WordIsP2)(Word word);
#define WordIsP2(word) ((word) > 0 && ((word) & ((word) - 1)) == 0)

/* Formatted Output -- see <design/writef>, <code/mpm.c> */

extern Res WriteF(mps_lib_FILE *stream, Count depth, ...);
extern Res WriteF_v(mps_lib_FILE *stream, Count depth, va_list args);
extern Res WriteF_firstformat_v(mps_lib_FILE *stream, Count depth,
                                const char *firstformat, va_list args);
#define WriteFYesNo(condition) ((WriteFS)((condition) ? "YES" : "NO"))


/* Miscellaneous support -- see <code/mpm.c> */

extern size_t StringLength(const char *s);
extern Bool StringEqual(const char *s1, const char *s2);

extern unsigned Random32(void);
extern Word RandomWord(void);

typedef Compare QuickSortCompare(void *left, void *right,
                                 void *closure);
extern void QuickSort(void *array[], Count length,
                      QuickSortCompare compare, void *closure,
                      SortStruct *sortStruct);


/* Version Determination
 *
 * <design/version-library>.  */

extern char *MPSVersion(void);


/* Pool Interface -- see <code/pool.c> */

extern Res PoolInit(Pool pool, Arena arena, PoolClass klass, ArgList args);
extern void PoolFinish(Pool pool);
extern Bool PoolClassCheck(PoolClass klass);
extern Bool PoolCheck(Pool pool);
extern Res PoolDescribe(Pool pool, mps_lib_FILE *stream, Count depth);

/* Must be thread-safe. <design/interface-c#.thread-safety>. */
#define PoolArena(pool)         ((pool)->arena)
#define PoolAlignment(pool)     ((pool)->alignment)
#define PoolSegRing(pool)       (&(pool)->segRing)
#define PoolArenaRing(pool) (&(pool)->arenaRing)
#define PoolOfArenaRing(node) RING_ELT(Pool, arenaRing, node)
#define PoolHasAttr(pool, Attr) ((ClassOfPoly(Pool, pool)->attr & (Attr)) != 0)
#define PoolSizeGrains(pool, size) ((size) >> (pool)->alignShift)
#define PoolGrainsSize(pool, grains) ((grains) << (pool)->alignShift)
#define PoolIndexOfAddr(base, pool, p) \
  (AddrOffset((base), (p)) >> (pool)->alignShift)
#define PoolAddrOfIndex(base, pool, i) AddrAdd(base, PoolGrainsSize(pool, i))

extern Bool PoolFormat(Format *formatReturn, Pool pool);

extern double PoolMutatorAllocSize(Pool pool);

extern Bool PoolOfAddr(Pool *poolReturn, Arena arena, Addr addr);
extern Bool PoolOfRange(Pool *poolReturn, Arena arena, Addr base, Addr limit);
extern Bool PoolHasAddr(Pool pool, Addr addr);
extern Bool PoolHasRange(Pool pool, Addr base, Addr limit);

extern Res PoolCreate(Pool *poolReturn, Arena arena, PoolClass klass,
                      ArgList args);
extern void PoolDestroy(Pool pool);
extern BufferClass PoolDefaultBufferClass(Pool pool);
extern Res PoolAlloc(Addr *pReturn, Pool pool, Size size);
extern void (PoolFree)(Pool pool, Addr old, Size size);
extern PoolGen PoolSegPoolGen(Pool pool, Seg seg);
extern Res PoolTraceBegin(Pool pool, Trace trace);
extern void PoolFreeWalk(Pool pool, FreeBlockVisitor f, void *p);
extern Size PoolTotalSize(Pool pool);
extern Size PoolFreeSize(Pool pool);

extern Res PoolAbsInit(Pool pool, Arena arena, PoolClass klass, ArgList arg);
extern void PoolAbsFinish(Inst inst);
extern Res PoolNoAlloc(Addr *pReturn, Pool pool, Size size);
extern Res PoolTrivAlloc(Addr *pReturn, Pool pool, Size size);
extern void PoolNoFree(Pool pool, Addr old, Size size);
extern void PoolTrivFree(Pool pool, Addr old, Size size);
extern PoolGen PoolNoSegPoolGen(Pool pool, Seg seg);
extern Res PoolNoBufferFill(Addr *baseReturn, Addr *limitReturn,
                            Pool pool, Buffer buffer, Size size);
extern Res PoolTrivBufferFill(Addr *baseReturn, Addr *limitReturn,
                              Pool pool, Buffer buffer, Size size);
extern void PoolNoBufferEmpty(Pool pool, Buffer buffer);
extern void PoolSegBufferEmpty(Pool pool, Buffer buffer);
extern void PoolTrivBufferEmpty(Pool pool, Buffer buffer);
extern Res PoolAbsDescribe(Inst inst, mps_lib_FILE *stream, Count depth);
extern Res PoolNoTraceBegin(Pool pool, Trace trace);
extern Res PoolTrivTraceBegin(Pool pool, Trace trace);
extern Res PoolNoScan(Bool *totalReturn, ScanState ss, Pool pool, Seg seg);
extern void PoolNoRampBegin(Pool pool, Buffer buf, Bool collectAll);
extern void PoolTrivRampBegin(Pool pool, Buffer buf, Bool collectAll);
extern void PoolNoRampEnd(Pool pool, Buffer buf);
extern void PoolTrivRampEnd(Pool pool, Buffer buf);
extern Res PoolNoFramePush(AllocFrame *frameReturn, Pool pool, Buffer buf);
extern Res PoolTrivFramePush(AllocFrame *frameReturn, Pool pool, Buffer buf);
extern Res PoolNoFramePop(Pool pool, Buffer buf, AllocFrame frame);
extern Res PoolTrivFramePop(Pool pool, Buffer buf, AllocFrame frame);
extern void PoolTrivFreeWalk(Pool pool, FreeBlockVisitor f, void *p);
extern PoolDebugMixin PoolNoDebugMixin(Pool pool);
extern BufferClass PoolNoBufferClass(void);
extern Size PoolNoSize(Pool pool);

/* See .critical.macros. */
#define PoolFreeMacro(pool, old, size) Method(Pool, pool, free)(pool, old, size)
#if !defined(AVER_AND_CHECK_ALL)
#define PoolFree(pool, old, size) PoolFreeMacro(pool, old, size)
#endif /* !defined(AVER_AND_CHECK_ALL) */

/* Abstract Pool Classes Interface -- see <code/poolabs.c> */
extern void PoolClassMixInBuffer(PoolClass klass);
extern void PoolClassMixInCollect(PoolClass klass);
DECLARE_CLASS(Inst, PoolClass, InstClass);
DECLARE_CLASS(Pool, AbstractPool, Inst);
DECLARE_CLASS(Pool, AbstractBufferPool, AbstractPool);
DECLARE_CLASS(Pool, AbstractSegBufPool, AbstractBufferPool);
typedef Pool AbstractCollectPool;
#define AbstractCollectPoolCheck PoolCheck
DECLARE_CLASS(Pool, AbstractCollectPool, AbstractSegBufPool);


/* Message Interface -- see <design/message> */
/* -- Internal (MPM) Interface -- functions for message originator */
extern Bool MessageCheck(Message message);
extern Bool MessageClassCheck(MessageClass klass);
extern Bool MessageTypeCheck(MessageType type);
extern void MessageInit(Arena arena, Message message,
                        MessageClass klass, MessageType type);
extern void MessageFinish(Message message);
extern Arena MessageArena(Message message);
extern Bool MessageOnQueue(Message message);
extern void MessagePost(Arena arena, Message message);
extern void MessageEmpty(Arena arena);
/* -- Delivery (Client) Interface -- functions for recipient */
extern void MessageTypeEnable(Arena arena, MessageType type);
extern void MessageTypeDisable(Arena arena, MessageType type);
extern Bool MessagePoll(Arena arena);
extern Bool MessageQueueType(MessageType *typeReturn, Arena arena);
extern Bool MessageGet(Message *messageReturn, Arena arena,
                       MessageType type);
extern void MessageDiscard(Arena arena, Message message);
/* -- Message Methods, Generic */
extern MessageType MessageGetType(Message message);
extern MessageClass MessageGetClass(Message message);
extern Clock MessageGetClock(Message message);
/* -- Message Method Dispatchers, Type-specific */
extern void MessageFinalizationRef(Ref *refReturn,
                                   Arena arena, Message message);
extern Size MessageGCLiveSize(Message message);
extern Size MessageGCCondemnedSize(Message message);
extern Size MessageGCNotCondemnedSize(Message message);
extern const char *MessageGCStartWhy(Message message);
/* -- Message Method Stubs, Type-specific */
extern void MessageNoFinalizationRef(Ref *refReturn,
                                     Arena arena, Message message);
extern Size MessageNoGCLiveSize(Message message);
extern Size MessageNoGCCondemnedSize(Message message);
extern Size MessageNoGCNotCondemnedSize(Message message);
extern const char *MessageNoGCStartWhy(Message message);


/* Trace Interface -- see <code/trace.c> */

#define TraceSetSingle(trace)       BS_SINGLE(TraceSet, (trace)->ti)
#define TraceSetIsSingle(ts)        BS_IS_SINGLE(ts)
#define TraceSetIsMember(ts, trace) BS_IS_MEMBER(ts, (trace)->ti)
#define TraceSetAdd(ts, trace)      BS_ADD(TraceSet, ts, (trace)->ti)
#define TraceSetDel(ts, trace)      BS_DEL(TraceSet, ts, (trace)->ti)
#define TraceSetUnion(ts1, ts2)     BS_UNION(ts1, ts2)
#define TraceSetInter(ts1, ts2)     BS_INTER(ts1, ts2)
#define TraceSetDiff(ts1, ts2)      BS_DIFF(ts1, ts2)
#define TraceSetSuper(ts1, ts2)     BS_SUPER(ts1, ts2)
#define TraceSetSub(ts1, ts2)       BS_SUB(ts1, ts2)
#define TraceSetComp(ts)            BS_COMP(ts)

#define TRACE_SET_ITER(ti, trace, ts, arena) \
  BEGIN \
    for (ti = 0; ti < TraceLIMIT; ++ti) { \
      trace = ArenaTrace(arena, ti); \
      if (TraceSetIsMember(ts, trace)) {

#define TRACE_SET_ITER_END(ti, trace, ts, arena) \
      } \
    } \
  END


extern void ScanStateInit(ScanState ss, TraceSet ts, Arena arena,
                          Rank rank, ZoneSet white);
extern void ScanStateInitSeg(ScanState ss, TraceSet ts, Arena arena,
                             Rank rank, ZoneSet white, Seg seg);
extern void ScanStateFinish(ScanState ss);
extern Bool ScanStateCheck(ScanState ss);
extern void ScanStateSetSummary(ScanState ss, RefSet summary);
extern RefSet ScanStateSummary(ScanState ss);
extern void ScanStateUpdateSummary(ScanState ss, Seg seg, Bool wasTotal);

/* See impl.h.mpmst.ss */
#define ScanStateZoneShift(ss)             ((Shift)(ss)->ss_s._zs)
#define ScanStateWhite(ss)                 ((ZoneSet)(ss)->ss_s._w)
#define ScanStateUnfixedSummary(ss)        ((RefSet)(ss)->ss_s._ufs)
#define ScanStateSetZoneShift(ss, shift)   ((void)((ss)->ss_s._zs = (shift)))
#define ScanStateSetWhite(ss, zs)          ((void)((ss)->ss_s._w = (zs)))
#define ScanStateSetUnfixedSummary(ss, rs) ((void)((ss)->ss_s._ufs = (rs)))

extern Bool TraceIdCheck(TraceId id);
extern Bool TraceSetCheck(TraceSet ts);
extern Bool TraceCheck(Trace trace);
extern Res TraceCreate(Trace *traceReturn, Arena arena, TraceStartWhy why);
extern void TraceDestroyInit(Trace trace);
extern void TraceDestroyFinished(Trace trace);

extern Bool TraceIsEmpty(Trace trace);
extern Res TraceAddWhite(Trace trace, Seg seg);
extern void TraceCondemnStart(Trace trace);
extern Res TraceCondemnEnd(double *mortalityReturn, Trace trace);
extern Res TraceStart(Trace trace, double mortality, double finishingTime);
extern Bool TracePoll(Work *workReturn, Bool *collectWorldReturn,
                      Globals globals, Bool collectWorldAllowed);

extern Rank TraceRankForAccess(Arena arena, Seg seg);
extern void TraceSegAccess(Arena arena, Seg seg, AccessSet mode);

extern void TraceAdvance(Trace trace);
extern Res TraceStartCollectAll(Trace *traceReturn, Arena arena, TraceStartWhy why);
extern Res TraceDescribe(Trace trace, mps_lib_FILE *stream, Count depth);

/* traceanc.c -- Trace Ancillary */

extern Bool TraceStartMessageCheck(TraceStartMessage message);
extern const char *TraceStartWhyToString(TraceStartWhy why);
extern void TracePostStartMessage(Trace trace);
extern Bool TraceMessageCheck(TraceMessage message);  /* trace end */
extern void TracePostMessage(Trace trace);  /* trace end */
extern Bool TraceIdMessagesCheck(Arena arena, TraceId ti);
extern Res TraceIdMessagesCreate(Arena arena, TraceId ti);
extern void TraceIdMessagesDestroy(Arena arena, TraceId ti);

/* Equivalent to <code/mps.h> MPS_SCAN_BEGIN */

#define TRACE_SCAN_BEGIN(ss) \
  BEGIN \
    /* Check range on zoneShift before casting to Shift. */ \
    AVER(ScanStateZoneShift(ss) < MPS_WORD_WIDTH); \
    { \
      Shift SCANzoneShift = ScanStateZoneShift(ss); \
      ZoneSet SCANwhite = ScanStateWhite(ss); \
      RefSet SCANsummary = ScanStateUnfixedSummary(ss); \
      Word SCANt; \
      mps_addr_t SCANref; \
      Res SCANres; \
      {

/* Equivalent to <code/mps.h> MPS_FIX1 */

#define TRACE_FIX1(ss, ref) \
  (SCANt = (Word)1 << ((Word)(ref) >> SCANzoneShift & (MPS_WORD_WIDTH-1)), \
   SCANsummary |= SCANt, \
   (SCANwhite & SCANt) != 0)

/* Equivalent to <code/mps.h> MPS_FIX2 */

/* TODO: The ref is copied to avoid breaking strict aliasing rules that could
   well affect optimised scan loops.  This code could be improved by
   returning the fixed ref as a result and using longjmp to signal errors,
   and that might well improve all scan loops too.  The problem is whether
   some embedded client platforms support longjmp.  RB 2012-09-07 */

#define TRACE_FIX2(ss, refIO) \
  (SCANref = (mps_addr_t)*(refIO), \
   SCANres = _mps_fix2(&(ss)->ss_s, &SCANref), \
   *(refIO) = SCANref, \
   SCANres)

/* Equivalent to <code/mps.h> MPS_FIX12 */

#define TRACE_FIX12(ss, refIO) \
  (TRACE_FIX1(ss, *(refIO)) ? TRACE_FIX2(ss, refIO) : ResOK)

/* Equivalent to <code/mps.h> MPS_SCAN_END */

#define TRACE_SCAN_END(ss) \
      } \
      ScanStateSetUnfixedSummary(ss, SCANsummary); \
    } \
  END

extern Res TraceScanFormat(ScanState ss, Addr base, Addr limit);
extern Res TraceScanArea(ScanState ss, Word *base, Word *limit,
                         mps_area_scan_t scan_area,
                         void *closure);
extern void TraceScanSingleRef(TraceSet ts, Rank rank, Arena arena,
                               Seg seg, Ref *refIO);


/* Arena Interface -- see <code/arena.c> */

DECLARE_CLASS(Inst, ArenaClass, InstClass);
DECLARE_CLASS(Arena, AbstractArena, Inst);
extern Bool ArenaClassCheck(ArenaClass klass);

extern Bool ArenaCheck(Arena arena);
extern Res ArenaCreate(Arena *arenaReturn, ArenaClass klass, ArgList args);
extern void ArenaDestroy(Arena arena);
extern Res ArenaDescribe(Arena arena, mps_lib_FILE *stream, Count depth);
extern Res ArenaDescribeTracts(Arena arena, mps_lib_FILE *stream, Count depth);
extern Bool ArenaAccess(Addr addr, AccessSet mode, MutatorContext context);
extern Res ArenaFreeLandInsert(Arena arena, Addr base, Addr limit);
extern Res ArenaFreeLandDelete(Arena arena, Addr base, Addr limit);

extern Bool GlobalsCheck(Globals arena);
extern Res GlobalsInit(Globals arena);
extern void GlobalsFinish(Globals arena);
extern Res GlobalsCompleteCreate(Globals arenaGlobals);
extern void GlobalsPrepareToDestroy(Globals arenaGlobals);
extern Res GlobalsDescribe(Globals arena, mps_lib_FILE *stream, Count depth);
extern void GlobalsArenaMap(void (*func)(Arena arena));
extern void GlobalsClaimAll(void);
extern void GlobalsReleaseAll(void);
extern void GlobalsReinitializeAll(void);

#define ArenaGlobals(arena) (&(arena)->globals)
#define GlobalsArena(glob) PARENT(ArenaStruct, globals, glob)

#define ArenaThreadRing(arena)  (&(arena)->threadRing)
#define ArenaDeadRing(arena)    (&(arena)->deadRing)
#define ArenaEpoch(arena)       (ArenaHistory(arena)->epoch) /* .epoch.ts */
#define ArenaTrace(arena, ti)   (&(arena)->trace[ti])
#define ArenaZoneShift(arena)   ((arena)->zoneShift)
#define ArenaStripeSize(arena)  ((Size)1 << ArenaZoneShift(arena))
#define ArenaGrainSize(arena)   ((arena)->grainSize)
#define ArenaGreyRing(arena, rank) (&(arena)->greyRing[rank])
#define ArenaPoolRing(arena) (&ArenaGlobals(arena)->poolRing)
#define ArenaChunkTree(arena) RVALUE((arena)->chunkTree)
#define ArenaChunkRing(arena)   (&(arena)->chunkRing)
#define ArenaShield(arena)      (&(arena)->shieldStruct)
#define ArenaHistory(arena)     (&(arena)->historyStruct)

extern Bool ArenaGrainSizeCheck(Size size);
#define AddrArenaGrainUp(addr, arena) AddrAlignUp(addr, ArenaGrainSize(arena))
#define AddrArenaGrainDown(addr, arena) AddrAlignDown(addr, ArenaGrainSize(arena))
#define AddrIsArenaGrain(addr, arena) AddrIsAligned(addr, ArenaGrainSize(arena))
#define SizeArenaGrains(size, arena) SizeAlignUp(size, ArenaGrainSize(arena))
#define SizeIsArenaGrains(size, arena) SizeIsAligned(size, ArenaGrainSize(arena))

extern void ArenaEnterLock(Arena arena, Bool recursive);
extern void ArenaLeaveLock(Arena arena, Bool recursive);

extern void ArenaEnter(Arena arena);
extern void ArenaLeave(Arena arena);
extern void (ArenaPoll)(Globals globals);

#if defined(SHIELD)
#elif defined(SHIELD_NONE)
#define ArenaPoll(globals)  UNUSED(globals)
#else
#error "No shield configuration."
#endif  /* SHIELD */

extern void ArenaEnterRecursive(Arena arena);
extern void ArenaLeaveRecursive(Arena arena);

extern Bool (ArenaStep)(Globals globals, double interval, double multiplier);
extern void ArenaClamp(Globals globals);
extern void ArenaRelease(Globals globals);
extern void ArenaPark(Globals globals);
extern void ArenaPostmortem(Globals globals);
extern Res ArenaStartCollect(Globals globals, TraceStartWhy why);
extern Res ArenaCollect(Globals globals, TraceStartWhy why);
extern Bool ArenaBusy(Arena arena);
extern Bool ArenaHasAddr(Arena arena, Addr addr);
extern void ArenaChunkInsert(Arena arena, Chunk chunk);
extern void ArenaChunkRemoved(Arena arena, Chunk chunk);
extern void ArenaAccumulateTime(Arena arena, Clock start, Clock now);

extern void ArenaSetEmergency(Arena arena, Bool emergency);
extern Bool ArenaEmergency(Arena arean);

extern Res ControlInit(Arena arena);
extern void ControlFinish(Arena arena);
extern Res ControlAlloc(void **baseReturn, Arena arena, size_t size);
extern void ControlFree(Arena arena, void *base, size_t size);
extern Res ControlDescribe(Arena arena, mps_lib_FILE *stream, Count depth);


/* Peek/Poke/Read/Write -- read/write possibly through barrier
 *
 * These are provided so that modules in the MPS can make occasional
 * access to client data, and to implement a software barrier for
 * segments that are not handed out to the mutator. They protect the
 * necessary colour, shield and summary invariants.
 *
 * Note that Peek and Poke can be called with an address that may or
 * may not be in memory managed by arena, whereas Read and Write
 * assert this is the case.
 */

/* Peek reads a value */
extern Ref ArenaPeek(Arena arena, Ref *p);
/* Same, but p known to be owned by arena */
extern Ref ArenaRead(Arena arena, Ref *p);
/* Same, but p must be in seg */
extern Ref ArenaPeekSeg(Arena arena, Seg seg, Ref *p);
/* Poke stores a value */
extern void ArenaPoke(Arena arena, Ref *p, Ref ref);
/* Same, but p known to be owned by arena */
extern void ArenaWrite(Arena arena, Ref *p, Ref ref);
/* Same, but p must be in seg */
extern void ArenaPokeSeg(Arena arena, Seg seg, Ref *p, Ref ref);


extern Size ArenaReserved(Arena arena);
extern Size ArenaCommitted(Arena arena);
extern Size ArenaSpareCommitted(Arena arena);
extern double ArenaSpare(Arena arena);
extern void ArenaSetSpare(Arena arena, double spare);
#define ArenaSpareCommitLimit(arena) ((Size)((double)ArenaCommitted(arena) * ArenaSpare(arena)))
#define ArenaCurrentSpare(arena) ((double)ArenaSpareCommitted(arena) / (double)ArenaCommitted(arena))

extern Size ArenaCommitLimit(Arena arena);
extern Res ArenaSetCommitLimit(Arena arena, Size limit);
extern double ArenaPauseTime(Arena arena);
extern void ArenaSetPauseTime(Arena arena, double pauseTime);
extern Size ArenaNoPurgeSpare(Arena arena, Size size);
extern Res ArenaNoGrow(Arena arena, LocusPref pref, Size size);

extern Size ArenaAvail(Arena arena);
extern Size ArenaCollectable(Arena arena);

extern Res ArenaExtend(Arena, Addr base, Size size);

extern void ArenaCompact(Arena arena, Trace trace);

extern Res ArenaFinalize(Arena arena, Ref obj);
extern Res ArenaDefinalize(Arena arena, Ref obj);

extern Res ArenaAlloc(Addr *baseReturn, LocusPref pref,
                      Size size, Pool pool);
extern Res ArenaFreeLandAlloc(Tract *tractReturn, Arena arena, ZoneSet zones,
                              Bool high, Size size, Pool pool);
extern void ArenaFree(Addr base, Size size, Pool pool);

extern Res ArenaNoExtend(Arena arena, Addr base, Size size);


/* Policy interface */

extern Res PolicyAlloc(Tract *tractReturn, Arena arena, LocusPref pref,
                       Size size, Pool pool);
extern Bool PolicyShouldCollectWorld(Arena arena, double availableTime,
                                     Clock now, Clock clocks_per_sec);
extern Bool PolicyStartTrace(Trace *traceReturn, Bool *collectWorldReturn,
                             Arena arena, Bool collectWorldAllowed);
extern Bool PolicyPoll(Arena arena);
extern Bool PolicyPollAgain(Arena arena, Clock start, Bool moreWork, Work tracedWork);


/* Locus interface */

extern Bool LocusPrefCheck(LocusPref pref);
extern LocusPref LocusPrefDefault(void);
extern void LocusPrefInit(LocusPref pref);
extern void LocusPrefExpress(LocusPref pref, LocusPrefKind kind, void *p);
extern Res LocusPrefDescribe(LocusPref pref, mps_lib_FILE *stream, Count depth);

extern void LocusInit(Arena arena);
extern void LocusFinish(Arena arena);
extern Bool LocusCheck(Arena arena);


/* Segment interface */

extern Res SegAlloc(Seg *segReturn, SegClass klass, LocusPref pref,
                    Size size, Pool pool,
                    ArgList args);
extern void SegFree(Seg seg);
extern Bool SegOfAddr(Seg *segReturn, Arena arena, Addr addr);
extern Bool SegFirst(Seg *segReturn, Arena arena);
extern Bool SegNext(Seg *segReturn, Arena arena, Seg seg);
extern Bool SegNextOfRing(Seg *segReturn, Arena arena, Pool pool, Ring next);
extern void SegSetWhite(Seg seg, TraceSet white);
extern void SegSetGrey(Seg seg, TraceSet grey);
extern void SegFlip(Seg seg, Trace trace);
extern void SegSetRankSet(Seg seg, RankSet rankSet);
extern void SegSetRankAndSummary(Seg seg, RankSet rankSet, RefSet summary);
extern Res SegMerge(Seg *mergedSegReturn, Seg segLo, Seg segHi);
extern Res SegSplit(Seg *segLoReturn, Seg *segHiReturn, Seg seg, Addr at);
extern Res SegAccess(Seg seg, Arena arena, Addr addr,
                     AccessSet mode, MutatorContext context);
extern Res SegWholeAccess(Seg seg, Arena arena, Addr addr,
                          AccessSet mode, MutatorContext context);
extern Res SegSingleAccess(Seg seg, Arena arena, Addr addr,
                           AccessSet mode, MutatorContext context);
extern Res SegWhiten(Seg seg, Trace trace);
extern void SegGreyen(Seg seg, Trace trace);
extern void SegBlacken(Seg seg, TraceSet traceSet);
extern Res SegScan(Bool *totalReturn, Seg seg, ScanState ss);
extern Res SegFix(Seg seg, ScanState ss, Addr *refIO);
extern Res SegFixEmergency(Seg seg, ScanState ss, Addr *refIO);
extern void SegReclaim(Seg seg, Trace trace);
extern void SegWalk(Seg seg, Format format, FormattedObjectsVisitor f,
                    void *v, size_t s);
extern Res SegAbsDescribe(Inst seg, mps_lib_FILE *stream, Count depth);
extern Res SegDescribe(Seg seg, mps_lib_FILE *stream, Count depth);
extern void SegSetSummary(Seg seg, RefSet summary);
extern Bool SegHasBuffer(Seg seg);
extern Bool SegBuffer(Buffer *bufferReturn, Seg seg);
extern void SegSetBuffer(Seg seg, Buffer buffer);
extern void SegUnsetBuffer(Seg seg);
extern Bool SegBufferFill(Addr *baseReturn, Addr *limitReturn,
                          Seg seg, Size size, RankSet rankSet);
extern Addr SegBufferScanLimit(Seg seg);
extern Bool SegCheck(Seg seg);
extern Bool GCSegCheck(GCSeg gcseg);
extern Bool SegClassCheck(SegClass klass);
DECLARE_CLASS(Inst, SegClass, InstClass);
DECLARE_CLASS(Seg, Seg, Inst);
DECLARE_CLASS(Seg, GCSeg, Seg);
DECLARE_CLASS(Seg, MutatorSeg, GCSeg);
#define SegGCSeg(seg) MustBeA(GCSeg, (seg))
extern void SegClassMixInNoSplitMerge(SegClass klass);

extern Size SegSize(Seg seg);
extern Addr (SegBase)(Seg seg);
extern Addr (SegLimit)(Seg seg);
#define SegBase(seg)            (TractBase((seg)->firstTract))
#define SegLimit(seg)           ((seg)->limit)
#define SegPool(seg)            (TractPool((seg)->firstTract))
/* .bitfield.promote: The bit field accesses need to be cast to the */
/* right type, otherwise they'll be promoted to signed int, see */
/* standard.ansic.6.2.1.1. */
#define SegRankSet(seg)         RVALUE((RankSet)(seg)->rankSet)
#define SegPM(seg)              RVALUE((AccessSet)(seg)->pm)
#define SegSM(seg)              RVALUE((AccessSet)(seg)->sm)
#define SegDepth(seg)           RVALUE((unsigned)(seg)->depth)
#define SegGrey(seg)            RVALUE((TraceSet)(seg)->grey)
#define SegWhite(seg)           RVALUE((TraceSet)(seg)->white)
#define SegNailed(seg)          RVALUE((TraceSet)(seg)->nailed)
#define SegPoolRing(seg)        (&(seg)->poolRing)
#define SegOfPoolRing(node)     RING_ELT(Seg, poolRing, (node))
#define SegOfGreyRing(node)     (&(RING_ELT(GCSeg, greyRing, (node)) \
                                   ->segStruct))

#define SegSummary(seg)         (((GCSeg)(seg))->summary)

#define SegSetPM(seg, mode)     ((void)((seg)->pm = BS_BITFIELD(Access, (mode))))
#define SegSetSM(seg, mode)     ((void)((seg)->sm = BS_BITFIELD(Access, (mode))))
#define SegSetDepth(seg, d)     ((void)((seg)->depth = BITFIELD(unsigned, (d), ShieldDepthWIDTH)))
#define SegSetNailed(seg, ts)   ((void)((seg)->nailed = BS_BITFIELD(Trace, (ts))))


/* Buffer Interface -- see <code/buffer.c> */

extern Res BufferCreate(Buffer *bufferReturn, BufferClass klass,
                        Pool pool, Bool isMutator, ArgList args);
extern void BufferDestroy(Buffer buffer);
extern Bool BufferCheck(Buffer buffer);
extern Bool SegBufCheck(SegBuf segbuf);
extern Res BufferDescribe(Buffer buffer, mps_lib_FILE *stream, Count depth);
extern Res BufferReserve(Addr *pReturn, Buffer buffer, Size size);
/* macro equivalent for BufferReserve, keep in sync with <code/buffer.c> */
/* TODO: Perhaps this isn't really necessary now that we build the MPS with
   more global optimisation and inlining. RB 2012-09-07 */
#define BUFFER_RESERVE(pReturn, buffer, size) \
  (AddrAdd(BufferAlloc(buffer), size) > BufferAlloc(buffer) && \
   AddrAdd(BufferAlloc(buffer), size) <= (Addr)BufferAP(buffer)->limit ? \
     (*(pReturn) = BufferAlloc(buffer), \
      BufferAP(buffer)->alloc = AddrAdd(BufferAlloc(buffer), size), \
      ResOK) : \
   BufferFill(pReturn, buffer, size))

extern Res BufferFill(Addr *pReturn, Buffer buffer, Size size);

extern Bool BufferCommit(Buffer buffer, Addr p, Size size);
/* macro equivalent for BufferCommit, keep in sync with <code/buffer.c> */
/* TODO: Perhaps this isn't really necessary now that we build the MPS with
 more global optimisation and inlining. RB 2012-09-07 */
#define BUFFER_COMMIT(buffer, p, size) \
  (BufferAP(buffer)->init = BufferAlloc(buffer), \
   BufferAP(buffer)->limit != 0 || BufferTrip(buffer, p, size))

extern Bool BufferTrip(Buffer buffer, Addr p, Size size);
extern void BufferFinish(Buffer buffer);
extern Bool BufferIsReset(Buffer buffer);
extern Bool BufferIsReady(Buffer buffer);
extern Bool BufferIsMutator(Buffer buffer);
extern void BufferSetAllocAddr(Buffer buffer, Addr addr);
extern void BufferAttach(Buffer buffer,
                         Addr base, Addr limit, Addr init, Size size);
extern void BufferDetach(Buffer buffer, Pool pool);
extern void BufferFlip(Buffer buffer);

extern mps_ap_t (BufferAP)(Buffer buffer);
#define BufferAP(buffer)        (&(buffer)->ap_s)
extern Buffer BufferOfAP(mps_ap_t ap);
#define BufferOfAP(ap)          PARENT(BufferStruct, ap_s, ap)

#define BufferArena(buffer) ((buffer)->arena)
#define BufferPool(buffer)  ((buffer)->pool)

extern Seg BufferSeg(Buffer buffer);

extern RankSet BufferRankSet(Buffer buffer);
extern void BufferSetRankSet(Buffer buffer, RankSet rankset);

#define BufferBase(buffer)      ((buffer)->base)
#define BufferGetInit(buffer) /* see .trans.bufferinit */ \
  ((Addr)(BufferAP(buffer)->init))
#define BufferAlloc(buffer)     ((Addr)(BufferAP(buffer)->alloc))
#define BufferLimit(buffer)     ((buffer)->poolLimit)
extern Addr BufferScanLimit(Buffer buffer);

extern void BufferReassignSeg(Buffer buffer, Seg seg);

extern Bool BufferIsTrapped(Buffer buffer);

extern void BufferRampBegin(Buffer buffer, AllocPattern pattern);
extern Res BufferRampEnd(Buffer buffer);
extern void BufferRampReset(Buffer buffer);

extern Res BufferFramePush(AllocFrame *frameReturn, Buffer buffer);
extern Res BufferFramePop(Buffer buffer, AllocFrame frame);

extern Bool BufferClassCheck(BufferClass klass);
DECLARE_CLASS(Inst, BufferClass, InstClass);
DECLARE_CLASS(Buffer, Buffer, Inst);
DECLARE_CLASS(Buffer, SegBuf, Buffer);
typedef Buffer RankBuf;
#define RankBufCheck BufferCheck
DECLARE_CLASS(Buffer, RankBuf, SegBuf);

extern AllocPattern AllocPatternRamp(void);
extern AllocPattern AllocPatternRampCollectAll(void);


/* FindDelete -- see <code/land.c> */

extern Bool FindDeleteCheck(FindDelete findDelete);


/* Format Interface -- see <code/format.c> */

extern Bool FormatCheck(Format format);
extern Res FormatCreate(Format *formatReturn, Arena arena, ArgList args);
extern void FormatDestroy(Format format);
extern Arena FormatArena(Format format);
extern Res FormatDescribe(Format format, mps_lib_FILE *stream, Count depth);
extern mps_res_t FormatNoScan(mps_ss_t mps_ss, mps_addr_t base, mps_addr_t limit);


/* Reference Interface -- see <code/ref.c> */

extern Bool RankCheck(Rank rank);
extern Bool RankSetCheck(RankSet rankSet);

#define RankSetIsMember(rs, r)  BS_IS_MEMBER((rs), (r))
#define RankSetSingle(r)        BS_SINGLE(RankSet, (r))
#define RankSetIsSingle(r)      BS_IS_SINGLE(r)
#define RankSetUnion(rs1, rs2)  BS_UNION((rs1), (rs2))
#define RankSetDel(rs, r)       BS_DEL(RankSet, (rs), (r))

#define AddrZone(arena, addr) \
  (((Word)(addr) >> (arena)->zoneShift) & (MPS_WORD_WIDTH - 1))

#define RefSetUnion(rs1, rs2)   BS_UNION((rs1), (rs2))
#define RefSetInter(rs1, rs2)   BS_INTER((rs1), (rs2))
#define RefSetDiff(rs1, rs2)    BS_DIFF((rs1), (rs2))
#define RefSetAdd(arena, rs, addr) \
  BS_ADD(RefSet, rs, AddrZone(arena, addr))
#define RefSetIsMember(arena, rs, addr) \
  BS_IS_MEMBER(rs, AddrZone(arena, addr))
#define RefSetSuper(rs1, rs2)   BS_SUPER((rs1), (rs2))
#define RefSetSub(rs1, rs2)     BS_SUB((rs1), (rs2))


/* Zone sets -- see <design/refset> */

#define ZoneSetUnion(zs1, zs2) BS_UNION(zs1, zs2)
#define ZoneSetInter(zs1, zs2) BS_INTER(zs1, zs2)
#define ZoneSetDiff(zs1, zs2)  BS_DIFF(zs1, zs2)
#define ZoneSetAddAddr(arena, zs, addr) \
  BS_ADD(ZoneSet, zs, AddrZone(arena, addr))
#define ZoneSetHasAddr(arena, zs, addr) \
  BS_IS_MEMBER(zs, AddrZone(arena, addr))
#define ZoneSetIsSingle(zs)    BS_IS_SINGLE(zs)
#define ZoneSetSub(zs1, zs2)   BS_SUB(zs1, zs2)
#define ZoneSetSuper(zs1, zs2) BS_SUPER(zs1, zs2)
#define ZoneSetComp(zs)        BS_COMP(zs)
#define ZoneSetIsMember(zs, z) BS_IS_MEMBER(zs, z)


extern ZoneSet ZoneSetOfRange(Arena arena, Addr base, Addr limit);
extern ZoneSet ZoneSetOfSeg(Arena arena, Seg seg);
typedef Bool (*RangeInZoneSet)(Addr *baseReturn, Addr *limitReturn,
                               Addr base, Addr limit,
                               Arena arena, ZoneSet zoneSet, Size size);
extern Bool RangeInZoneSetFirst(Addr *baseReturn, Addr *limitReturn,
                                Addr base, Addr limit,
                                Arena arena, ZoneSet zoneSet, Size size);
extern Bool RangeInZoneSetLast(Addr *baseReturn, Addr *limitReturn,
                               Addr base, Addr limit,
                               Arena arena, ZoneSet zoneSet, Size size);
extern ZoneSet ZoneSetBlacklist(Arena arena);


/* Shield Interface -- see <code/shield.c> */

extern void ShieldInit(Shield shield);
extern void ShieldFinish(Shield shield);
extern Bool ShieldCheck(Shield shield);
extern Res ShieldDescribe(Shield shield, mps_lib_FILE *stream, Count depth);
extern void ShieldDestroyQueue(Shield shield, Arena arena);
extern void (ShieldRaise)(Arena arena, Seg seg, AccessSet mode);
extern void (ShieldLower)(Arena arena, Seg seg, AccessSet mode);
extern void (ShieldEnter)(Arena arena);
extern void (ShieldLeave)(Arena arena);
extern void (ShieldExpose)(Arena arena, Seg seg);
extern void (ShieldCover)(Arena arena, Seg seg);
extern void (ShieldHold)(Arena arena);
extern void (ShieldRelease)(Arena arena);
extern void (ShieldFlush)(Arena arena);

#if defined(SHIELD)
/* Nothing to do: functions declared in all shield configurations. */
#elif defined(SHIELD_NONE)
#define ShieldRaise(arena, seg, mode) \
  BEGIN UNUSED(arena); UNUSED(seg); UNUSED(mode); END
#define ShieldLower(arena, seg, mode) \
  BEGIN UNUSED(arena); UNUSED(seg); UNUSED(mode); END
#define ShieldEnter(arena) BEGIN UNUSED(arena); END
#define ShieldLeave(arena) AVER(arena->busyTraces == TraceSetEMPTY)
#define ShieldExpose(arena, seg)  \
  BEGIN UNUSED(arena); UNUSED(seg); END
#define ShieldCover(arena, seg) \
  BEGIN UNUSED(arena); UNUSED(seg); END
#define ShieldHold(arena) BEGIN UNUSED(arena); END
#define ShieldRelease(arena) BEGIN UNUSED(arena); END
#define ShieldFlush(arena) BEGIN UNUSED(arena); END
#else
#error "No shield configuration."
#endif  /* SHIELD */


/* Location Dependency -- see <code/ld.c> */

extern void HistoryInit(History history);
extern void HistoryFinish(History);
extern Res HistoryDescribe(History history, mps_lib_FILE *stream, Count depth);
extern Bool HistoryCheck(History history);
extern void LDReset(mps_ld_t ld, Arena arena);
extern void LDAdd(mps_ld_t ld, Arena arena, Addr addr);
extern Bool LDIsStaleAny(mps_ld_t ld, Arena arena);
extern Bool LDIsStale(mps_ld_t ld, Arena arena, Addr addr);
extern void LDAge(Arena arena, RefSet moved);
extern void LDMerge(mps_ld_t ld, Arena arena, mps_ld_t from);


/* Root Interface -- see <code/root.c> */

extern Res RootCreateArea(Root *rootReturn, Arena arena,
                          Rank rank, RootMode mode,
                          Word *base, Word *limit,
                          mps_area_scan_t scan_area,
                          void *closure);
extern Res RootCreateAreaTagged(Root *rootReturn, Arena arena,
                                Rank rank, RootMode mode,
                                Word *base, Word *limit,
                                mps_area_scan_t scan_area,
                                Word mask, Word pattern);
extern Res RootCreateThread(Root *rootReturn, Arena arena,
                            Rank rank, Thread thread,
                            mps_area_scan_t scan_area,
                            void *closure,
                            Word *stackCold);
extern Res RootCreateThreadTagged(Root *rootReturn, Arena arena,
                                  Rank rank, Thread thread,
                                  mps_area_scan_t scan_area,
                                  Word mask, Word pattern,
                                  Word *stackCold);
extern Res RootCreateFmt(Root *rootReturn, Arena arena,
                           Rank rank, RootMode mode,
                           mps_fmt_scan_t scan,
                           Addr base, Addr limit);
extern Res RootCreateFun(Root *rootReturn, Arena arena,
                        Rank rank, mps_root_scan_t scan,
                        void *p, size_t s);
extern void RootDestroy(Root root);
extern Bool RootModeCheck(RootMode mode);
extern Bool RootCheck(Root root);
extern Res RootDescribe(Root root, mps_lib_FILE *stream, Count depth);
extern Res RootsDescribe(Globals arenaGlobals, mps_lib_FILE *stream, Count depth);
extern Rank RootRank(Root root);
extern AccessSet RootPM(Root root);
extern RefSet RootSummary(Root root);
extern void RootGrey(Root root, Trace trace);
extern Res RootScan(ScanState ss, Root root);
extern Arena RootArena(Root root);
extern Bool RootOfAddr(Root *root, Arena arena, Addr addr);
extern void RootAccess(Root root, AccessSet mode);
typedef Res (*RootIterateFn)(Root root, void *p);
extern Res RootsIterate(Globals arena, RootIterateFn f, void *p);


/* Land Interface -- see <design/land> */

extern Bool LandCheck(Land land);
#define LandArena(land) ((land)->arena)
#define LandAlignment(land) ((land)->alignment)
extern Size (LandSize)(Land land);
extern Res LandInit(Land land, LandClass klass, Arena arena, Align alignment, void *owner, ArgList args);
extern void LandFinish(Land land);
extern Res (LandInsert)(Range rangeReturn, Land land, Range range);
extern Res LandInsertSteal(Range rangeReturn, Land land, Range rangeIO);
extern Res (LandDelete)(Range rangeReturn, Land land, Range range);
extern Res LandDeleteSteal(Range rangeReturn, Land land, Range range);
extern Bool (LandIterate)(Land land, LandVisitor visitor, void *closure);
extern Bool (LandIterateAndDelete)(Land land, LandDeleteVisitor visitor, void *closure);
extern Bool (LandFindFirst)(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete);
extern Bool (LandFindLast)(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete);
extern Bool (LandFindLargest)(Range rangeReturn, Range oldRangeReturn, Land land, Size size, FindDelete findDelete);
extern Res (LandFindInZones)(Bool *foundReturn, Range rangeReturn, Range oldRangeReturn, Land land, Size size, ZoneSet zoneSet, Bool high);
extern Res LandDescribe(Land land, mps_lib_FILE *stream, Count depth);
extern Bool LandFlushVisitor(Bool *deleteReturn, Land land, Range range, void *closure);
extern Bool (LandFlush)(Land dest, Land src);
extern Size LandSlowSize(Land land);
extern Bool LandClassCheck(LandClass klass);

/* See .critical.macros. */
#define LandSizeMacro(land) Method(Land, land, sizeMethod)(land)
#define LandInsertMacro(rangeReturn, land, range) Method(Land, land, insert)(rangeReturn, land, range)
#define LandDeleteMacro(rangeReturn, land, range) Method(Land, land, delete)(rangeReturn, land, range)
#define LandIterateMacro(land, visitor, closure) Method(Land, land, iterate)(land, visitor, closure)
#define LandIterateAndDeleteMacro(land, visitor, closure) Method(Land, land, iterateAndDelete)(land, visitor, closure)
#define LandFindFirstMacro(rangeReturn, oldRangeReturn, land, size, findDelete) Method(Land, land, findFirst)(rangeReturn, oldRangeReturn, land, size, findDelete)
#define LandFindLastMacro(rangeReturn, oldRangeReturn, land, size, findDelete) Method(Land, land, findLast)(rangeReturn, oldRangeReturn, land, size, findDelete)
#define LandFindLargestMacro(rangeReturn, oldRangeReturn, land, size, findDelete) Method(Land, land, findLargest)(rangeReturn, oldRangeReturn, land, size, findDelete)
#define LandFindInZonesMacro(foundReturn, rangeReturn, oldRangeReturn, land, size, zoneSet, high) Method(Land, land, findInZones)(foundReturn, rangeReturn, oldRangeReturn, land, size, zoneSet, high)
#define LandFlushMacro(dest, src) LandIterateAndDelete(src, LandFlushVisitor, dest)
#if !defined(AVER_AND_CHECK_ALL)
#define LandSize(land) LandSizeMacro(land)
#define LandInsert(rangeReturn, land, range) LandInsertMacro(rangeReturn, land, range)
#define LandDelete(rangeReturn, land, range) LandDeleteMacro(rangeReturn, land, range)
#define LandIterate(land, visitor, closure) LandIterateMacro(land, visitor, closure)
#define LandIterateAndDelete(land, visitor, closure) LandIterateAndDeleteMacro(land, visitor, closure)
#define LandFindFirst(rangeReturn, oldRangeReturn, land, size, findDelete) LandFindFirstMacro(rangeReturn, oldRangeReturn, land, size, findDelete)
#define LandFindLast(rangeReturn, oldRangeReturn, land, size, findDelete) LandFindLastMacro(rangeReturn, oldRangeReturn, land, size, findDelete)
#define LandFindLargest(rangeReturn, oldRangeReturn, land, size, findDelete) LandFindLargestMacro(rangeReturn, oldRangeReturn, land, size, findDelete)
#define LandFindInZones(foundReturn, rangeReturn, oldRangeReturn, land, size, zoneSet, high) LandFindInZonesMacro(foundReturn, rangeReturn, oldRangeReturn, land, size, zoneSet, high)
#define LandFlush(dest, src) LandFlushMacro(dest, src)
#endif /* !defined(AVER_AND_CHECK_ALL) */

DECLARE_CLASS(Inst, LandClass, InstClass);
DECLARE_CLASS(Land, Land, Inst);


/* STATISTIC -- gather statistics (in some varieties)
 *
 * <design/diag#.stat>.
 */

#if defined(STATISTICS)

#define STATISTIC(gather) BEGIN gather; END
#define STATISTIC_WRITE(format, arg) (format), (arg),

#elif defined(STATISTICS_NONE)

#define STATISTIC(gather) NOOP
#define STATISTIC_WRITE(format, arg)

#else /* !defined(STATISTICS) && !defined(STATISTICS_NONE) */

#error "No statistics configured."

#endif

#endif /* mpm_h */


/* C. COPYRIGHT AND LICENSE
 *
 * Copyright (C) 2001-2020 Ravenbrook Limited <https://www.ravenbrook.com/>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
