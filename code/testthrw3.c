/* testthrw3.c: MULTI-THREADED TEST IMPLEMENTATION (WINDOWS)
 *
 * $Id$
 * Copyright (c) 2014-2020 Ravenbrook Limited.  See end of file for license.
 */

#include "testlib.h"
#include "testthr.h"

static DWORD WINAPI testthr_start(LPVOID arg)
{
  testthr_t *thread = arg;
  thread->result = (*thread->start)(thread->arg);
  return 0;
}

void testthr_create(testthr_t *thread_o, testthr_routine_t start, void *arg)
{
  HANDLE res;
  thread_o->start = start;
  thread_o->arg = arg;
  res = CreateThread(NULL, 0, testthr_start, thread_o, 0, NULL);
  if (res == NULL)
    error("CreateThread failed with error %lu",
          (unsigned long)GetLastError());
  else
    thread_o->handle = res;
}

void testthr_join(testthr_t *thread, void **result_o)
{
  DWORD res = WaitForSingleObject(thread->handle, INFINITE);
  if (res != WAIT_OBJECT_0)
    error("WaitForSingleObject failed with result %lu (error %lu)",
          (unsigned long)res, (unsigned long)GetLastError());
  if (result_o)
    *result_o = thread->result;
}

void testthr_mutex_init(testthr_mutex_t *mutex)
{
  InitializeCriticalSection(mutex);
}

void testthr_mutex_finish(testthr_mutex_t *mutex)
{
  DeleteCriticalSection(mutex);
}

void testthr_mutex_lock(testthr_mutex_t *mutex)
{
  EnterCriticalSection(mutex);
}

void testthr_mutex_unlock(testthr_mutex_t *mutex)
{
  LeaveCriticalSection(mutex);
}


/* C. COPYRIGHT AND LICENSE
 *
 * Copyright (C) 2014-2020 Ravenbrook Limited <https://www.ravenbrook.com/>.
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
