/*
 * Copyright (c) 2016 Eric B. Decker
 * Copyright (c) 2009 Stanford University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * These definitions should work for both M3 and M4 ARM Cortex processors.
 *
 * Includes interrupt enable/disable routines for nesC.
 *
 * @author Wanja Hofer <wanja@cs.fau.de>
 * @author Thomas Schmid
 * @author Eric B. Decker <cire831@gmail.com>
 */

#ifndef __CORTEXM_NESC_H__
#define __CORTEXM_NESC_H__

typedef uint32_t __nesc_atomic_t;

/*
 * A brief word on Interrupts and Barrier instructions.
 * (see B5.2.3, page B5-735, ARMv7-M ARM, DDI0403E.b)
 * M3 and M4 are both v7-Ms.
 *
 * On entry to a critical region (ie. atomic_start)
 *
 *   Disabling interrupts is increasing execution priority, and the
 *   MSR or CPSIE instruction serializes the change to the instruction
 *   stream.  I think that means that any instructions following the
 *   disable will not start actual execution until the disable has
 *   finished executing.
 *
 * Conclusion: No barrier instruction is needed when disabling.
 *
 * On exit from a critical region....
 *
 *   Enabling interrupts is decreasing the execution priority.  The new
 *   priority is visible to instructions executed after the enable only if
 *   an ISB is executed or exception entry/exit.
 *
 *   This only matters if one wants to allow for an interrupt to get in
 *   immediately after interrupts are reenabled.  One could put an ISB
 *   following the CPSIE instruction.
 *
 *   This however flushes the pipeline, wasting the time it took to fetch
 *   the next instructions following the enable.  This isn't very expensive
 *   (2 instruction times?).  However, it happens at every atomic section
 *   exit.  The interrupt on the other hand may or may not be there.
 *
 *   If one doesn't put the ISB in the instruction stream, then the
 *   following instructions will start to be fetched and then the interrupt
 *   is acked (if it is present).  This means that we will always start to
 *   execute 1 possibly 2 more instructions after the interrupt enable
 *   before the interrupt gets in.  This increases the interrupt latency by
 *   a small amount (a few instruction fetch times).  Verses the time it
 *   takes to flush the pipeline above.  Pretty much a wash.
 *
 * Conclusion: Barrier isn't needed and doesn't seem to be worth taking the
 * extra pipeline hit for every enable for an interrupt that may or may not
 * be there.  The cost is possibly slightly increased interrupt latency
 * (approx 2 instruction times).
 *
 * The gcc memory barriers keep the compiler from reordering which could
 * mess up what is in the pipeline.  These barriers are needed.
 */

/*
 * this is what "atomic" entry uses to disable interrupts
 */
inline __nesc_atomic_t __nesc_atomic_start() @spontaneous() __attribute__((always_inline)) {
  __nesc_atomic_t oldState = 0;

  asm volatile(
    "mrs %[old], primask\n"
    "cpsid i\n"
    : [old] "=&r" (oldState) // output, assure write only!
    :                        // input
    : "cc", "memory"         // clobber condition code flag and memory barrier
               );
  return oldState;
}

inline void __nesc_atomic_end(__nesc_atomic_t oldState) @spontaneous() __attribute__((always_inline)) {
  asm volatile("" : : : "memory"); // memory barrier
  asm volatile("msr primask, %[old]\n"
    :                      // no output
    : [old] "r" (oldState) // input
    : "memory" );
}

inline void __nesc_enable_interrupt() __attribute__((always_inline)) {
  asm volatile("cpsie i" : : : "memory");
}

inline void __nesc_disable_interrupt() __attribute__((always_inline)) {
  asm volatile("cpsid i" : : : "memory");
}

#endif          /* __CORTEXM_NESC_H__ */
