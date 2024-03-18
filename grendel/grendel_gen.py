# =========================================================================== #
# User Inputs
# =========================================================================== #
yes_choices = ['yes', 'y']
no_choices = ['no', 'n']

# =========================================================================== #
# BDD Packages and their supported Diagrams.
# =========================================================================== #
from enum import Enum

dd_t = Enum('dd_t', ['bdd', 'zdd'])

dd_choice = []
for dd in dd_t:
    if input(f"Include '{dd.name.upper()}' benchmarks? (yes/No): ").lower() in yes_choices:
        dd_choice.append(dd)

if not dd_choice:
    print("\n  At least one kind of Decision Diagram should be included!")
    exit(1)

package_t = Enum('package_t', ['adiar', 'buddy', 'cal', 'cudd', 'libbdd', 'sylvan'])

package_dd = {
    package_t.adiar  : [dd_t.bdd, dd_t.zdd],
    package_t.buddy  : [dd_t.bdd],
    package_t.cal    : [dd_t.bdd],
    package_t.cudd   : [dd_t.bdd, dd_t.zdd],
    package_t.libbdd : [dd_t.bdd],
    package_t.sylvan : [dd_t.bdd]
}

print("")

package_choice = []

for p in package_t:
    if any(dd in package_dd[p] for dd in dd_choice):
        if input(f"Include '{p.name}' package? (yes/No): ").lower() in yes_choices:
            package_choice.append(p)

if not package_choice:
    print("\n  At least one Library should be included!")
    exit(1)

bdd_packages = [p for p in package_choice if dd_t.bdd in package_dd[p]] if dd_t.bdd in dd_choice else []
zdd_packages = [p for p in package_choice if dd_t.zdd in package_dd[p]] if dd_t.zdd in dd_choice else []

print("\nPackages")
print("  BDD:", [p.name for p in bdd_packages])
print("  ZDD:", [p.name for p in zdd_packages])

# =========================================================================== #
# Benchmark Instances
# =========================================================================== #

# --------------------------------------------------------------------------- #
# For the Picotrav benchmarks, we need to obtain the 'depth' and 'size'
# optimised circuit for each of the given spec circuits.
# --------------------------------------------------------------------------- #
import os

epfl_spec_t    = Enum('epfl_spec_t',    ['arithmetic', 'random_control'])
epfl_opt_t     = Enum('epfl_opt_t',     ['depth', 'size'])
picotrav_opt_t = Enum('picotrav_opt_t', ['DF', 'INPUT', 'LEVEL', 'LEVEL_DF', 'RANDOM'])

def picotrav__spec(spec_t, circuit_name):
    return f"../epfl/{spec_t.name}/{circuit_name}.blif"

def picotrav__opt(opt_t, circuit_name):
    circuit_file = [f for f
                      in os.listdir(f"../../epfl/best_results/{opt_t.name}")
                      if f.startswith(circuit_name)][0]
    return f"../epfl/best_results/{opt_t.name}/{circuit_file}"

def picotrav__args(spec_t, opt_t, circuit_name, picotrav_opt):
    return f"-o {picotrav_opt.name} -f {picotrav__spec(spec_t, circuit_name)} -f {picotrav__opt(opt_t, circuit_name)}"

def qbf__args(circuit_name):
    # All of Irfansha Shaik's circuits seem to be output in a depth-first order.
    return f"-f ../SAT2023_GDDL/QBF_instances/{circuit_name}.qcir -o df"

# --------------------------------------------------------------------------- #
# Since we are testing BDD packages over such a wide spectrum, we have some
# instances that require several days of computaiton time (closing into the 15
# days time limit of the q48 nodes). Yet, the SLURM scheduler does (for good
# reason) not give high priority to jobs with a 15 days time limit. Hence, for
# every instance we should try and schedule it with a time limit that reflects
# the actual computation time(ish).
#
# The following is a list of all instances including their timings.
# --------------------------------------------------------------------------- #
BENCHMARKS = {
    # Benchmark Name
    #    dd_t
    #        Time Limit, Benchmark Arguments
    # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  #
    #        [DD,HH,MM], "-? ..."
    # ------------------------------------------------------------------------ #

    # --------------------------------------------------------------------------
    "game-of-life": {
        dd_t.bdd: [
            # All solutions
            [ [ 0, 0,10], "-o none -N 3 -N 3" ],
            [ [ 0, 0,10], "-o none -N 4 -N 3" ],
            [ [ 0, 3,00], "-o none -N 4 -N 4" ],
            [ [ 0, 1,00], "-o none -N 5 -N 4" ],
            [ [ 0, 4,00], "-o none -N 5 -N 5" ],
            [ [ 0,12,00], "-o none -N 6 -N 5" ],
            [ [ 8, 0,00], "-o none -N 6 -N 6" ],
            # Mirror Vertical
            [ [ 0, 0,10], "-o mirror-vertical -N 3 -N 3" ],
            [ [ 0, 0,10], "-o mirror-vertical -N 4 -N 3" ],
            [ [ 0, 0,30], "-o mirror-vertical -N 4 -N 4" ],
            [ [ 0, 0,30], "-o mirror-vertical -N 5 -N 4" ],
            [ [ 0, 1,00], "-o mirror-vertical -N 5 -N 5" ],
            [ [ 0, 1,00], "-o mirror-vertical -N 6 -N 5" ],
            [ [ 0, 4,00], "-o mirror-vertical -N 6 -N 6" ],
            [ [ 1, 0,00], "-o mirror-vertical -N 7 -N 6" ],
            [ [ 2, 0,00], "-o mirror-vertical -N 7 -N 7" ],
            # Mirror Quadrant
            [ [ 0, 0,10], "-o mirror-quad -N 3 -N 3" ],
            [ [ 0, 0,10], "-o mirror-quad -N 4 -N 3" ],
            [ [ 0, 0,10], "-o mirror-quad -N 4 -N 4" ],
            [ [ 0, 0,10], "-o mirror-quad -N 5 -N 4" ],
            [ [ 0, 0,10], "-o mirror-quad -N 5 -N 5" ],
            [ [ 0, 0,30], "-o mirror-quad -N 6 -N 5" ],
            [ [ 0, 1,00], "-o mirror-quad -N 6 -N 6" ],
            [ [ 2, 0,00], "-o mirror-quad -N 7 -N 6" ],
            # Mirror Diagonal
            [ [ 0, 0,10], "-o mirror-diagonal -N 3 -N 3" ],
            [ [ 0, 0,10], "-o mirror-diagonal -N 4 -N 4" ],
            [ [ 0, 0,30], "-o mirror-diagonal -N 5 -N 5" ],
            [ [ 0, 2,00], "-o mirror-diagonal -N 6 -N 6" ],
            [ [ 2, 0,00], "-o mirror-diagonal -N 6 -N 6" ],
            # Mirror Double Diagonal
            [ [ 0, 0,10], "-o mirror-double_diagonal -N 3 -N 3" ],
            [ [ 0, 0,30], "-o mirror-double_diagonal -N 4 -N 4" ],
            [ [ 0, 0,30], "-o mirror-double_diagonal -N 5 -N 5" ],
            [ [ 0, 3,00], "-o mirror-double_diagonal -N 6 -N 6" ],
            [ [ 9, 0,00], "-o mirror-double_diagonal -N 7 -N 7" ],
            # Rotate 90
            [ [ 0, 0,10], "-o rotate-90 -N 3 -N 3" ],
            [ [ 0, 0,30], "-o rotate-90 -N 4 -N 4" ],
            [ [ 0, 1,00], "-o rotate-90 -N 5 -N 5" ],
            [ [ 0, 2,00], "-o rotate-90 -N 6 -N 6" ],
            [ [15, 0,00], "-o rotate-90 -N 7 -N 7" ], # Adiar : 1+ TiB disk usage before quantification!
            # Rotate 180
            [ [ 0, 0,10], "-o rotate-180 -N 3 -N 3" ],
            [ [ 0, 0,10], "-o rotate-180 -N 4 -N 3" ],
            [ [ 0, 0,30], "-o rotate-180 -N 4 -N 4" ],
            [ [ 0, 1,00], "-o rotate-180 -N 5 -N 4" ],
            [ [ 0, 2,00], "-o rotate-180 -N 5 -N 5" ],
            [ [ 0, 4,00], "-o rotate-180 -N 6 -N 5" ],
            [ [ 2, 0,00], "-o rotate-180 -N 6 -N 6" ],
        ]
    },
    # --------------------------------------------------------------------------
    "hamiltonian": {
        dd_t.bdd: [
            # Binary Encoding
            [ [ 0, 0,10], "-o binary -N 4 -N 3" ],
            [ [ 0, 0,10], "-o binary -N 4 -N 4" ],
            [ [ 0, 0,10], "-o binary -N 5 -N 4" ],
            [ [ 0, 0,30], "-o binary -N 6 -N 5" ],
            [ [ 0, 1,00], "-o binary -N 6 -N 6" ],
            [ [ 0, 2,00], "-o binary -N 7 -N 6" ],
            [ [ 0,12,00], "-o binary -N 8 -N 6" ],
            [ [ 2, 0,00], "-o binary -N 8 -N 7" ],
          # [ [15, 0,00], "-o binary -N 8 -N 8" ],
        ],
        dd_t.zdd: [
            # Time-based Encoding
            [ [ 0, 0,10], "-o time -N 4 -N 3" ],
            [ [ 0, 0,10], "-o time -N 4 -N 4" ],
            [ [ 0, 0,10], "-o time -N 5 -N 4" ],
            [ [ 0, 0,10], "-o time -N 6 -N 5" ],
            [ [ 0, 0,10], "-o time -N 6 -N 6" ],
            [ [ 0, 0,30], "-o time -N 7 -N 6" ],
            [ [ 0, 1,00], "-o time -N 8 -N 6" ],
            [ [ 0, 1,00], "-o time -N 8 -N 7" ],
          # [ [15, 0,00], "-o time -N 8 -N 8" ],
        ]
    },
    # --------------------------------------------------------------------------
    "picotrav": {
        dd_t.bdd: [
            # arithmetic
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "adder",      picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "adder",      picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "bar",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "bar",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "div",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "div",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "log2",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "log2",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "max",        picotrav_opt_t.LEVEL_DF) ],
          # [ [ 6,12, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "max",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "multiplier", picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "multiplier", picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 3, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sin",        picotrav_opt_t.LEVEL_DF) ],
            [ [ 5, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sin",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sqrt",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sqrt",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "square",     picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "square",     picotrav_opt_t.LEVEL_DF) ],
            # random_control
            [ [ 0, 1, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "arbiter",    picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "arbiter",    picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "mem_ctrl",   picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "mem_ctrl",   picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "router",     picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "router",     picotrav_opt_t.INPUT) ],
            [ [ 1, 0,00], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "voter",      picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 3,00], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "voter",      picotrav_opt_t.LEVEL_DF) ],
        ],
        dd_t.zdd: [
            # arithmetic
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "adder",      picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "adder",      picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "bar",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "bar",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "div",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "div",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "log2",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "log2",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "max",        picotrav_opt_t.LEVEL_DF) ],
            [ [ 9, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "max",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "multiplier", picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "multiplier", picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 3, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sin",        picotrav_opt_t.LEVEL_DF) ],
            [ [ 5, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sin",        picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sqrt",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sqrt",       picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "square",     picotrav_opt_t.LEVEL_DF) ],
          # [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "square",     picotrav_opt_t.LEVEL_DF) ],
            # random_control
            [ [ 0, 1, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "arbiter",    picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "arbiter",    picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 1, 0, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "mem_ctrl",   picotrav_opt_t.LEVEL_DF) ],
            [ [ 1, 0, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "mem_ctrl",   picotrav_opt_t.LEVEL_DF) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "router",     picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "router",     picotrav_opt_t.INPUT) ],
            [ [ 3, 0, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.size,  "voter",      picotrav_opt_t.LEVEL_DF) ],
            [ [ 2, 0, 0], picotrav__args(epfl_spec_t.random_control, epfl_opt_t.depth, "voter",      picotrav_opt_t.LEVEL_DF) ],
        ]
    },
    # --------------------------------------------------------------------------
    "qbf": {
        dd_t.bdd: [
            # B/ (Breakthrough)
            [ [ 0, 0,30], qbf__args("B/2x4_13_bwnib") ],
            [ [ 0, 1,00], qbf__args("B/2x5_17_bwnib") ],
            [ [ 0, 0,30], qbf__args("B/2x6_15_bwnib") ],
            [ [ 0, 6,00], qbf__args("B/3x4_19_bwnib") ],
            [ [ 0, 0,10], qbf__args("B/3x5_11_bwnib") ],
            [ [ 0, 0,10], qbf__args("B/3x6_9_bwnib") ],
            # BSP/ (Breakthrough 2nd Player)
            [ [ 0, 0,10], qbf__args("BSP/2x4_8_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/3x4_12_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/2x5_10_bwnib") ],
            [ [ 0, 0,30], qbf__args("BSP/3x5_12_bwnib") ],
            [ [ 0, 0,30], qbf__args("BSP/2x6_14_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/3x6_10_bwnib") ],
            # C4/ (Connect 4)
            [ [ 0, 0,10], qbf__args("C4/2x2_3_connect2_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/3x3_3_connect2_bwnib") ],
            [ [ 0, 6,00], qbf__args("C4/3x3_9_connect3_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/4x4_3_connect2_bwnib") ],
            [ [ 0, 2,00], qbf__args("C4/4x4_9_connect3_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/4x4_15_connect4_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/5x5_3_connect2_bwnib") ],
            [ [ 6, 0,00], qbf__args("C4/5x5_9_connect3_bwnib") ],
            [ [15, 0,00], qbf__args("C4/5x5_11_connect4_bwnib") ], # Adiar : 1+ TiB disk usage during quantification!
            [ [ 0, 0,10], qbf__args("C4/6x6_3_connect2_bwnib") ],
            [ [ 8, 0,00], qbf__args("C4/6x6_9_connect3_bwnib") ],
            [ [15, 0,00], qbf__args("C4/6x6_11_connect4_bwnib") ], # Adiar : 1+ TiB disk usage!
            # D/ (Domineering)
            [ [ 0, 0,10], qbf__args("D/2x2_2_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/2x3_4_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/2x4_4_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/2x5_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/2x6_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/3x2_2_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/3x3_4_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/3x4_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/3x6_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/3x5_8_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/4x2_5_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/4x3_7_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/4x4_8_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/4x5_11_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/4x6_12_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x2_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x3_8_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x4_10_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x5_13_bwnib") ],
            [ [ 0, 0,30], qbf__args("D/5x6_11_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/6x2_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/6x3_10_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/6x4_12_bwnib") ],
            [ [ 0, 0,30], qbf__args("D/6x5_11_bwnib") ],
            [ [ 0,12,00], qbf__args("D/6x6_11_bwnib") ],
            # EP/ (Evader-Pursuer)
            [ [ 0, 0,10], qbf__args("EP/4x4_3_e-4-1_p-2-3_bwnib") ],
            [ [ 0, 3,00], qbf__args("EP/4x4_21_e-4-1_p-1-2_bwnib") ],
            [ [ 0, 0,10], qbf__args("EP/8x8_7_e-8-1_p-3-4_bwnib") ],
            [ [ 0, 2,00], qbf__args("EP/8x8_11_e-8-1_p-2-3_bwnib") ],
            # EP-dual/ (Evader-Pursuer from the other's perspective)
            [ [ 0, 0,10], qbf__args("EP-dual/4x4_2_e-4-1_p-1-2_bwnib") ],
            [ [ 0, 0,10], qbf__args("EP-dual/4x4_10_e-4-1_p-2-3_bwnib") ],
            [ [ 0, 0,10], qbf__args("EP-dual/8x8_6_e-8-1_p-2-3_bwnib") ],
            [ [ 0, 4,00], qbf__args("EP-dual/8x8_10_e-8-1_p-3-4_bwnib") ],
            # hex/ (Hex)
            [ [ 0, 0,10], qbf__args("hex/browne_5x5_07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/browne_5x5_09_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_02_5x5-11_bwnib") ],
            [ [ 0, 2,00], qbf__args("hex/hein_02_5x5-13_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_04_3x3-03_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_04_3x3-05_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_06_4x4-11_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_06_4x4-13_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_07_4x4-07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_07_4x4-09_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_08_5x5-09_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_08_5x5-11_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_09_4x4-05_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_09_4x4-07_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_10_5x5-11_bwnib") ],
            [ [ 0, 2,00], qbf__args("hex/hein_10_5x5-13_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_11_5x5-09_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_11_5x5-11_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_12_4x4-05_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_12_4x4-07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_13_5x5-07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_13_5x5-09_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_14_5x5-07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_14_5x5-09_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_15_5x5-13_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_15_5x5-15_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_16_5x5-11_bwnib") ],
            [ [ 0, 1,00], qbf__args("hex/hein_16_5x5-13_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_19_5x5-09_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_19_5x5-11_bwnib") ],
            # httt/ (High-dimensional Tic-Tac-Toe)
            [ [ 0, 0,10], qbf__args("httt/3x3_3_domino_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_5_el_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_elly_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_fatty_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_knobby_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_tic_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_tippy_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_11_knobby_bwnib") ],
            [ [ 0, 0,30], qbf__args("httt/4x4_13_skinny_bwnib") ],
            [ [ 0, 0,30], qbf__args("httt/4x4_15_fatty_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_3_domino_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_5_el_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_5_tic_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_7_elly_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_9_tippy_bwnib") ],
        ]
    },
    # --------------------------------------------------------------------------
    "queens": {
        dd_t.bdd: [
            [ [ 0, 0,10], "-N 4"  ],
            [ [ 0, 0,10], "-N 5"  ],
            [ [ 0, 0,10], "-N 6"  ],
            [ [ 0, 0,10], "-N 7"  ],
            [ [ 0, 0,10], "-N 8"  ],
            [ [ 0, 0,10], "-N 9"  ],
            [ [ 0, 0,10], "-N 10" ],
            [ [ 0, 0,30], "-N 11" ],
            [ [ 0, 1, 0], "-N 12" ],
            [ [ 0, 2, 0], "-N 13" ],
            [ [ 0, 3, 0], "-N 14" ],
            [ [ 0, 3, 0], "-N 15" ],
            [ [ 0,20, 0], "-N 16" ],
            [ [ 3, 0, 0], "-N 17" ],
            [ [15, 0, 0], "-N 18" ],
         ],
        dd_t.zdd: [
            [ [ 0, 0,10], "-N 4"  ],
            [ [ 0, 0,10], "-N 5"  ],
            [ [ 0, 0,10], "-N 6"  ],
            [ [ 0, 0,10], "-N 7"  ],
            [ [ 0, 0,10], "-N 8"  ],
            [ [ 0, 0,10], "-N 9"  ],
            [ [ 0, 0,10], "-N 10" ],
            [ [ 0, 0,30], "-N 11" ],
            [ [ 0, 1, 0], "-N 12" ],
            [ [ 0, 2, 0], "-N 13" ],
            [ [ 0, 3, 0], "-N 14" ],
            [ [ 0, 3, 0], "-N 15" ],
            [ [ 0,20, 0], "-N 16" ],
            [ [ 3, 0, 0], "-N 17" ],
            [ [15, 0, 0], "-N 18" ],
        ]
    },
    # --------------------------------------------------------------------------
    "tic-tac-toe": {
        dd_t.bdd: [
            [ [ 0, 0,10], "-N 13" ],
            [ [ 0, 0,10], "-N 14" ],
            [ [ 0, 0,10], "-N 15" ],
            [ [ 0, 0,10], "-N 16" ],
            [ [ 0, 0,10], "-N 17" ],
            [ [ 0, 0,10], "-N 18" ],
            [ [ 0, 0,10], "-N 19" ],
            [ [ 0, 0,10], "-N 20" ],
            [ [ 0, 0,30], "-N 21" ],
            [ [ 0, 2, 0], "-N 22" ],
            [ [ 0, 4, 0], "-N 23" ],
            [ [ 0,12, 0], "-N 24" ],
            [ [ 1,12, 0], "-N 25" ],
            [ [ 4, 0, 0], "-N 26" ],
            [ [ 6, 0, 0], "-N 27" ],
            [ [ 8, 0, 0], "-N 28" ],
            [ [15,00, 0], "-N 29" ],
        ],
        dd_t.zdd: [
            [ [ 0, 0,10], "-N 13" ],
            [ [ 0, 0,10], "-N 14" ],
            [ [ 0, 0,10], "-N 15" ],
            [ [ 0, 0,10], "-N 16" ],
            [ [ 0, 0,10], "-N 17" ],
            [ [ 0, 0,10], "-N 18" ],
            [ [ 0, 0,10], "-N 19" ],
            [ [ 0, 0,10], "-N 20" ],
            [ [ 0, 0,30], "-N 21" ],
            [ [ 0, 2, 0], "-N 22" ],
            [ [ 0, 4, 0], "-N 23" ],
            [ [ 0,12, 0], "-N 24" ],
            [ [ 1,12, 0], "-N 25" ],
            [ [ 4, 0, 0], "-N 26" ],
            [ [ 6, 0, 0], "-N 27" ],
            [ [ 9, 0, 0], "-N 28" ],
            [ [15,00, 0], "-N 29" ],
        ]
    },
}

print("")

benchmark_choice = []
for b in BENCHMARKS.keys():
    if any(dd in BENCHMARKS[b].keys() for dd in dd_choice):
        if input(f"Include '{b}' Benchmark? (yes/No): ").lower() in yes_choices:
            benchmark_choice.append(b)

bdd_benchmarks = [b for b in benchmark_choice if dd_t.bdd in BENCHMARKS[b].keys()] if dd_t.bdd in dd_choice else []
zdd_benchmarks = [b for b in benchmark_choice if dd_t.zdd in BENCHMARKS[b].keys()] if dd_t.zdd in dd_choice else []

print("\nBenchmarks")
print("  BDD: ", bdd_benchmarks)
print("  ZDD: ", zdd_benchmarks)

print("")

# --------------------------------------------------------------------------- #
# To get these benchmarks to not flood the SLURM manager, we need to group them
# together by their time limit (creating an array of jobs for each time limit).
# --------------------------------------------------------------------------- #
try:
    time_factor = float(input("Time Limit Factor (default: 1.0): "))
except:
    time_factor = 1.0

def time_limit_scale(t):
    hours_to_mins = 60
    days_to_mins = 24 * hours_to_mins

    total_minutes = t[0] * days_to_mins + t[1] * hours_to_mins + t[2]
    scaled_minutes = time_factor * total_minutes
    return [
        int(scaled_minutes / days_to_mins),
        int((scaled_minutes % days_to_mins) / hours_to_mins),
        int(scaled_minutes % hours_to_mins)
    ]

def time_limit_str(t):
    minutes = t[2]
    if minutes < 10:
        minutes = f"0{minutes}"

    hours = t[1]
    if hours < 10:
        hours = f"0{hours}"

    days = t[0]
    if days < 10:
        days = f"0{days}"

    return f"{days}-{hours}:{minutes}:{0}{0}"

grouped_instances = {}

for benchmark in BENCHMARKS:
    if benchmark not in benchmark_choice: continue

    for dd in BENCHMARKS[benchmark]:
        if dd not in dd_choice: continue

        instances = BENCHMARKS[benchmark][dd]
        for instance in instances:
            for p in package_t:
                if p not in package_choice: continue

                if dd in package_dd[p]:
                    grouped_instances.setdefault(time_limit_str(time_limit_scale(instance[0])), []).append([p, benchmark, dd, instance[1]])

print("")

# --------------------------------------------------------------------------- #
# For each benchmark, we need to derive a unique name. This is used for the
# executable and output file.
# --------------------------------------------------------------------------- #

def executable(package, benchmark, dd):
    return f"{package.name}_{benchmark}_{dd.name}"

def benchmark_uid(package, benchmark, dd, args):
    args_suffix = '_'.join(args.replace('-', '').replace(' ', '_').split('/')[-1:]).split('.')[0][-32:]

    return [package.name, benchmark, dd.name, args_suffix]

def output_path(package, benchmark, dd, args):
    b = benchmark_uid(package, benchmark, dd, args)
    return f"out/{b[0]}/{b[1]}/{b[2]}/{b[3]}.out"

# =========================================================================== #
# Script Strings
# =========================================================================== #

partitions = {
             # Mem, CPU
    "q20":    [128, "ivybridge"],
    "q20fat": [128, "ivybridge"],
    "q24":    [256, "haswell"],
    "q28":    [256, "broadwell"],
    "q36":    [384, "skylake"],
    "q40":    [384, "cascadelake"],
    "q48":    [384, "cascadelake"],
    "q64":    [512, "icelake-server"],
}

partition = "q48"
partition_choice = input("Grendel Node (default: 'q48'): ")
if partition_choice:
    if not partition_choice in partitions.keys():
        print(f"Partition '{partition_choice}' is unknown")
        exit(-1)
    partition = partition_choice

MODULE_LOAD = '''module load gcc/10.1.0
module load cmake/3.23.5 autoconf/2.71 automake/1.16.1
module load boost/1.68.0 gmp/6.2.1'''

ENV_SETUP = '''export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++
export LC_ALL=C'''

def sbatch_str(jobname, time, is_exclusive):
    return f'''#SBATCH --job-name={jobname}
#SBATCH --partition={partition}
#SBATCH --mem={"0" if is_exclusive else "16G"}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time={time}
#SBATCH --mail-type=END,FAIL,REQUEUE
#SBATCH --mail-user=soelvsten@cs.au.dk''' + ("\n#SBATCH --exclusive" if is_exclusive else "")

def benchmark_awk_str(i):
    # $1  = output file path
    d1 = output_path(i[0], i[1], i[2], i[3])

    # $2  = executable
    d2 = executable(i[0], i[1], i[2])

    # $3+ = arguments
    ds = i[3]

    return f"{d1} {d2} {ds}"

SLURM_ARRAY_ID = "$SLURM_ARRAY_TASK_ID"
SLURM_JOB_ID   = "$SLURM_JOB_ID"
SLURM_ORIGIN   = "$SLURM_SUBMIT_DIR"

def benchmark_str(time, benchmarks):
    current_dir = os.getcwd()
    parent_dir  = os.path.dirname(current_dir)
    parent_dir_name = os.path.basename(parent_dir)

    slurm_job_prefix = parent_dir_name
    slurm_job_suffix = time.replace(':','-')

    # Array file to be read with AWK
    awk_content = '\n'.join(list(map(benchmark_awk_str, benchmarks)))
    awk_name    = slurm_job_suffix + ".awk"

    # SLURM Shell Script
    awk_array_idx  = f"NR == '{SLURM_ARRAY_ID}'"

    args_length = max(map(lambda b : len(b[3].split()), benchmarks))
    awk_args = '" "$' + '" "$'.join(map(lambda b : str(b), range(3, args_length+3)))

    memory = (partitions[partition][0] - 16) * 1024

    slurm_content = f'''#!/bin/bash
{sbatch_str(f"{slurm_job_prefix}__{slurm_job_suffix}", time, True)}
#SBATCH --array=1-{len(benchmarks)}

awk '{awk_array_idx} {{ system("touch {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\n=========  Started `date`  ==========\\n\\" | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("{SLURM_ORIGIN}/build/src/"$2 {awk_args} " -M {memory} -t /scratch/{SLURM_JOB_ID} 2>&1 | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\nexit code: \\"$? | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\n=========  Finished `date`  ==========\\n\\" | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}
'''
    slurm_name = slurm_job_suffix + ".sh"

    # Return name and both file's content
    return [[slurm_name, slurm_content], [awk_name, awk_content]]

CMAKE_STATS        = "BDD_BENCHMARK_STATS"
CMAKE_GRENDEL_FLAG = "BDD_BENCHMARK_GRENDEL"

def build_str(stats):
    cpu = partitions[partition][1]

    prefix = f'''#!/bin/bash
echo -e "\\n=========  Started `date`  ==========\\n"

{MODULE_LOAD}
{ENV_SETUP}

# Build
echo "Build"
mkdir -p ./build && cd ./build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_FLAGS="-march={cpu}" -D CMAKE_CXX_FLAGS="-march={cpu}" -D {CMAKE_GRENDEL_FLAG}=ON -D {CMAKE_STATS}={"ON" if stats else "OFF"} ..
'''

    bdd_build = ""
    if bdd_benchmarks:
        assert(bdd_packages)
        bdd_build = f'''
echo ""
echo "Build BDD Benchmarks"
for package in {' '.join([p.name for p in bdd_packages])} ; do
		for benchmark in {' '.join([b for b in bdd_benchmarks])} ; do
			mkdir -p ../out/$package ; \\
			mkdir -p ../out/$package/$benchmark ; \\
			mkdir -p ../out/$package/$benchmark/bdd ; \\
			make $package'_'$benchmark'_bdd' ;
		done ;
done
'''

    zdd_build = ""
    if zdd_benchmarks:
        assert(zdd_packages)
        zdd_build = f'''
echo ""
echo "Build ZDD Benchmarks"
for package in {' '.join([p.name for p in zdd_packages])} ; do
		for benchmark in {' '.join([b for b in zdd_benchmarks])} ; do
			mkdir -p ../out/$package ; \\
			mkdir -p ../out/$package/$benchmark ; \\
			mkdir -p ../out/$package/$benchmark/zdd ; \\
			make $package'_'$benchmark'_zdd' ;
		done ;
done
'''

    suffix = f'''
echo -e "\\n========= Finished `date` ==========\\n"
'''

    return prefix + bdd_build + zdd_build + suffix

# =========================================================================== #
# Run Script Strings and Save to Disk
# =========================================================================== #
print("")

with open("build.sh", "w") as file:
    file.write(build_str(input(f"Include Statistics? (yes/No): ").lower() in yes_choices))

for (t,b) in grouped_instances.items():
    for [filename, content] in benchmark_str(t,b):
        with open(filename, "w") as file:
            file.write(content)

print("\nScripts")
print("  Time Limits:      ", len(grouped_instances.keys()))
print("  Minimum Array:    ", min(map(lambda x : len(x[1]), grouped_instances.items())))
print("  Maximum Array:    ", max(map(lambda x : len(x[1]), grouped_instances.items())))
print("  Total Benchmarks: ", sum(map(lambda x : len(x[1]), grouped_instances.items())))
