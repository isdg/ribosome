
RIBOSOME - PLAN
===============

A RISC-V RV32I CPU built in C from NAND gates up.

Structural at the ALU, behavioral above it at first. Every gate bottoms
out in one nand() call, which counts itself - that count is both the
cost model and a test oracle.

-------------------------------------------------------------------------------
LAYOUT
-------------------------------------------------------------------------------

    Makefile        two configs: debug (sanitizers), release (-O2)
    src/            gate, bus, alu, then datapath; headers beside sources
    tests/          one file per module, mirrored names
    third_party/    vendored single headers, if any
    .md/            this file

-------------------------------------------------------------------------------
STAGES
-------------------------------------------------------------------------------

    1  ALU          nand -> gates -> adder -> ALU
    2  DATAPATH     register file, PC, memory, decoder; single-cycle
    3  ASSEMBLER    text -> machine code, ELF32 loader
    4  PIPELINE     IF/ID/EX/MEM/WB, hazards, forwarding, prediction
    5  PRIVILEGED   CSRs, M/S/U modes, traps, Sv32; boot xv6-riscv

-------------------------------------------------------------------------------
STAGE 1 - ALU
-------------------------------------------------------------------------------

Gate costs are asserted, not just measured. A construction that gets
cheaper without getting simpler has stopped going through the gates.

    nand          1    the only primitive; a truth table, not an expression
    _not          1    done
    _and          2    done
    _or           3    done
    _xor          4    done
    mux           4    done
    half adder    6
    full adder   15
    add n=32    480    32 full adders, ripple carry
    add/sub     608    one _xor per bit on b, cin = sub
    shifter     640    barrel: 5 stages x 32 muxes

Next:

    bus.h/bus.c   bus_from_u32, bus_to_u32, bus_copy. The only place
                  above nand where host operators are legal - it
                  marshals integers into wires, it does not compute.

    adder         half -> full -> ripple. Expose the carry *into* the
                  top bit, not only the carry out: SLT needs
                  sign XOR overflow, overflow = cout_msb XOR cin_msb.

    alu           ten ops in funct3 order:
                  ADD SUB SLL SLT SLTU XOR SRL SRA OR AND
                  All ten evaluate every time; a mux tree picks one.
                  There is no `if` in hardware. SLTU is free - it is
                  NOT of the subtract's carry-out.

Parameterize every bus function on width n. That is what makes the
exhaustive test possible at all.

-------------------------------------------------------------------------------
VERIFICATION
-------------------------------------------------------------------------------

Four tiers, cheapest first:

    unit          truth tables. Total, not sampled - every gate takes at
                  most 3 bits. 52 checks in tests/test_gate.c.
    cost          gate counts. Catches `return a ^ b;`, which no
                  correctness test can ever see.
    oracle        exhaustive 8-bit sweep: 256 x 256 x 10 ops against
                  native C. Edge cases + random at 32-bit.
    conformance   riscv-tests, then Spike lockstep - all 32 registers
                  and PC compared after every instruction, halt on the
                  first divergence. Stage 2 onward.

The rule the cost tier enforces: no &, |, ^ or + on bit values anywhere
in src/, except inside nand() and bus marshalling. Grep-able.

-------------------------------------------------------------------------------
SPEC
-------------------------------------------------------------------------------

RISC-V Instruction Set Manual, github.com/riscv/riscv-isa-manual ->
Releases. Take the latest ratified PDF; main is a moving draft.

    Vol I ch.2      RV32I base. ~20 pages. The whole ISA. Stages 1-4.
    Vol I listings  opcode encoding tables. Print these.
    Vol II          CSRs, modes, traps, paging. Stage 5.

Six instruction formats - R I S B U J - and that is the whole decoder.
The B and J immediates are bit-scrambled on purpose: bit 31 is always
the sign, so sign extension is one fanout wire, not a crossbar.

-------------------------------------------------------------------------------
TOOLCHAIN
-------------------------------------------------------------------------------

    > brew install riscv64-elf-gcc     # in core, no tap needed
    > brew install riscv64-elf-gdb     # from stage 3

Spike is not in homebrew core - needs the riscv-software-src/riscv tap
or a source build. Not needed until stage 2.

-------------------------------------------------------------------------------
NOTES
-------------------------------------------------------------------------------

  - Registers are uint32_t everywhere; cast only at the point of
    comparison. Signed overflow is UB and -O2 will delete the path.
  - Mask shift amounts to 5 bits. Shifting by >= width is UB in C.
  - Code is capped at 80 columns.
