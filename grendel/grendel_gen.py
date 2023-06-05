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
    if input(f"Include '{dd.name.upper()}' benchmarks? (yes/no): ").lower() in yes_choices:
        dd_choice.append(dd)

package_t = Enum('package_t', ['adiar', 'buddy', 'cal', 'cudd', 'sylvan'])

package_dd = {
    package_t.adiar  : [dd_t.bdd, dd_t.zdd],
    package_t.buddy  : [dd_t.bdd],
    package_t.cal    : [dd_t.bdd],
    package_t.cudd   : [dd_t.bdd, dd_t.zdd],
    package_t.sylvan : [dd_t.bdd]
}

print("")

package_choice = []
for p in package_t:
    if any(dd in package_dd[p] for dd in dd_choice):
        if input(f"Include '{p.name}' package? (yes/no): ").lower() in yes_choices:
            package_choice.append(p)

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
picotrav_opt_t = Enum('picotrav_opt_t', ['DFS', 'INPUT', 'LEVEL', 'LEVEL_DFS', 'RANDOM'])

def picotrav__spec(spec_t, circuit_name):
    return f"benchmarks/epfl/{spec_t.name}/{circuit_name}.blif"

def picotrav__opt(opt_t, circuit_name):
    circuit_file = [f for f
                      in os.listdir(f"../benchmarks/epfl/best_results/{opt_t.name}")
                      if f.startswith(circuit_name)][0]
    return f"benchmarks/epfl/best_results/{opt_t.name}/{circuit_file}"

def picotrav__args(spec_t, opt_t, circuit_name, picotrav_opt):
    return f"-o {picotrav_opt.name} -f {picotrav__spec(spec_t, circuit_name)} -f {picotrav__opt(opt_t, circuit_name)}"

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
            [ [ 9, 0, 0], "-N 17" ],
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
    "tic_tac_toe": {
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
    # --------------------------------------------------------------------------
    "knights_tour": {
        dd_t.zdd: [
            # Open Tours
            [ [ 0, 0,10], "-o OPEN -N 1" ],
            [ [ 0, 0,10], "-o OPEN -N 2" ],
            [ [ 0, 0,10], "-o OPEN -N 3" ],
            [ [ 0, 0,10], "-o OPEN -N 4" ],
            [ [ 0, 0,10], "-o OPEN -N 5" ],
            [ [ 0, 0,10], "-o OPEN -N 6" ],
            [ [ 0, 0,10], "-o OPEN -N 7" ],
            [ [ 0, 0,10], "-o OPEN -N 8" ],
            [ [ 0, 0,10], "-o OPEN -N 9" ],
            [ [ 0, 0,10], "-o OPEN -N 1" ],
            [ [ 0, 1, 0], "-o OPEN -N 1" ],
            [ [ 0, 4, 0], "-o OPEN -N 1" ],
            [ [ 8, 0, 0], "-o OPEN -N 1" ],
            [ [15, 0, 0], "-o OPEN -N 1" ],
            [ [15, 0, 0], "-o OPEN -N 1" ],
            [ [15, 0, 0], "-o OPEN -N 1" ],
            # Closed Tours
            [ [ 0, 0,10], "-o CLOSED -N 1" ],
            [ [ 0, 0,10], "-o CLOSED -N 2" ],
            [ [ 0, 0,10], "-o CLOSED -N 3" ],
            [ [ 0, 0,10], "-o CLOSED -N 4" ],
            [ [ 0, 0,10], "-o CLOSED -N 5" ],
            [ [ 0, 0,10], "-o CLOSED -N 6" ],
            [ [ 0, 0,10], "-o CLOSED -N 7" ],
            [ [ 0, 0,10], "-o CLOSED -N 8" ],
            [ [ 0, 0,10], "-o CLOSED -N 9" ],
            [ [ 0, 0,10], "-o CLOSED -N 1" ],
            [ [ 0, 0,10], "-o CLOSED -N 1" ],
            [ [ 0, 0,30], "-o CLOSED -N 1" ],
            [ [ 0, 2, 0], "-o CLOSED -N 1" ],
            [ [ 0, 0,10], "-o CLOSED -N 1" ],
            [ [15, 0, 0], "-o CLOSED -N 1" ],
            [ [15, 0, 0], "-o CLOSED -N 1" ],
        ]
    },
    # --------------------------------------------------------------------------
    "picotrav": {
        dd_t.bdd: [
            # arithmetic
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "adder",      picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "adder",      picotrav_opt_t.LEVEL_DFS) ],
            [ [ 5, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "bar",        picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "bar",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "div",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "div",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "log2",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "log2",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "max",        picotrav_opt_t.LEVEL_DFS) ],
            [ [ 6,12, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "max",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "multiplier", picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "multiplier", picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 3, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sin",        picotrav_opt_t.LEVEL_DFS) ],
            [ [ 5, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sin",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sqrt",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sqrt",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "square",     picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "square",     picotrav_opt_t.LEVEL_DFS) ],
            # random_control
            [ [ 0, 1, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "arbiter",    picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "arbiter",    picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "mem_ctrl",   picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "mem_ctrl",   picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "router",     picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "router",     picotrav_opt_t.INPUT) ],
            [ [ 0, 2,00], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "voter",      picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 2,00], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "voter",      picotrav_opt_t.LEVEL_DFS) ],
        ],
        dd_t.zdd: [
            # arithmetic
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "adder",      picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "adder",      picotrav_opt_t.LEVEL_DFS) ],
            [ [ 5, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "bar",        picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "bar",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "div",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "div",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "log2",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "log2",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "max",        picotrav_opt_t.LEVEL_DFS) ],
            [ [ 6,12, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "max",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "multiplier", picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "multiplier", picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 3, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sin",        picotrav_opt_t.LEVEL_DFS) ],
            [ [ 5, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sin",        picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "sqrt",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "sqrt",       picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "square",     picotrav_opt_t.LEVEL_DFS) ],
            [ [15, 0, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "square",     picotrav_opt_t.LEVEL_DFS) ],
            # random_control
            [ [ 0, 1, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "arbiter",    picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "arbiter",    picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "cavlc",      picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "ctrl",       picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "dec",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "i2c",        picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "int2float",  picotrav_opt_t.INPUT) ],
            [ [ 0, 2, 0], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "mem_ctrl",   picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "mem_ctrl",   picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "priority",   picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "router",     picotrav_opt_t.INPUT) ],
            [ [ 0, 0,10], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "router",     picotrav_opt_t.INPUT) ],
            [ [ 0, 2,00], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.size,  "voter",      picotrav_opt_t.LEVEL_DFS) ],
            [ [ 0,12,00], picotrav__args(epfl_spec_t.arithmetic, epfl_opt_t.depth, "voter",      picotrav_opt_t.LEVEL_DFS) ],
        ]
    },
    # --------------------------------------------------------------------------
    "qbf": {
        dd_t.bdd: [
            # TODO
        ]
    },
}

print("")

benchmark_choice = []
for b in BENCHMARKS.keys():
    if any(dd in BENCHMARKS[b].keys() for dd in dd_choice):
        if input(f"Include '{b}' Benchmark? (yes/no): ").lower() in yes_choices:
            benchmark_choice.append(b)

bdd_benchmarks = [b for b in benchmark_choice if dd_t.bdd in BENCHMARKS[b].keys()] if dd_t.bdd in dd_choice else []
zdd_benchmarks = [b for b in benchmark_choice if dd_t.zdd in BENCHMARKS[b].keys()] if dd_t.zdd in dd_choice else []

print("\nBenchmarks")
print("  BDD: ", bdd_benchmarks)
print("  ZDD: ", zdd_benchmarks)

# --------------------------------------------------------------------------- #
# To get these benchmarks to not flood the SLURM manager, we need to group them
# together by their time limit (creating an array of jobs for each time limit).
# --------------------------------------------------------------------------- #
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
                    grouped_instances.setdefault(time_limit_str(instance[0]), []).append([p, benchmark, dd, instance[1]])

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

MODULE_LOAD = '''module load gcc/10.1.0
module load cmake/3.23.5 autoconf/2.71 automake/1.16.1
module load boost/1.68.0 gmp/6.2.1'''

ENV_SETUP = '''export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++
export LC_ALL=C'''

def sbatch_str(jobname, time, is_exclusive):
    return f'''#SBATCH --job-name={jobname}
#SBATCH --partition=q48
#SBATCH --mem=350G
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
    slurm_job_name = f"benchmarks_{time.replace(':','-')}"

    # Array file to be read with AWK
    awk_content = '\n'.join(list(map(benchmark_awk_str, benchmarks)))
    awk_name    = slurm_job_name + ".awk"

    # SLURM Shell Script
    awk_array_idx  = f"NR == '{SLURM_ARRAY_ID}'"

    args_length = max(map(lambda b : len(b[3].split()), benchmarks))
    awk_args = '" "$' + '" "$'.join(map(lambda b : str(b), range(3, args_length+3)))

    slurm_content = f'''#!/bin/bash
{sbatch_str(slurm_job_name, time, True)}
#SBATCH --array=1-{len(benchmarks)}

awk '{awk_array_idx} {{ system("touch {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\n=========  Started `date`  ==========\\n\\" | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("{SLURM_ORIGIN}/build/src/"$2 {awk_args} " -M {300*1024} -t /scratch/{SLURM_JOB_ID} 2>&1 | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\nexit code: \\"$? | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\n=========  Finished `date`  ==========\\n\\" | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}
'''
    slurm_name = slurm_job_name + ".sh"

    # Return name and both file's content
    return [[slurm_name, slurm_content], [awk_name, awk_content]]

def build_str():
    prefix = f'''#!/bin/bash
{sbatch_str("benchmarks_build", time_limit_str([ 0, 0,30]), False)}

echo -e "\\n=========  Started `date`  ==========\\n"

{MODULE_LOAD}
{ENV_SETUP}

# Build
echo "Build"
mkdir -p {SLURM_ORIGIN}/build/ && cd {SLURM_ORIGIN}/build/
cmake -D BDD_BENCHMARK_GRENDEL=ON {SLURM_ORIGIN}
'''

    cudd_build = ""
    if package_t.cudd in package_choice:
        cudd_build = f'''
echo ""
echo "Build CUDD"
cd {SLURM_ORIGIN}/external/cudd
autoreconf
./configure --prefix {SLURM_ORIGIN}/build/cudd/ --enable-obj
make && make install

cd {SLURM_ORIGIN}/build/
'''

    bdd_build = ""
    if bdd_benchmarks:
        assert(bdd_packages)
        bdd_build = f'''
echo ""
echo "Build BDD Benchmarks"
for package in {' '.join([p.name for p in bdd_packages])} ; do
		for benchmark in {' '.join([b for b in bdd_benchmarks])} ; do
			mkdir -p ../out/$package ; \
			mkdir -p ../out/$package/$benchmark ; \
			mkdir -p ../out/$package/$benchmark/bdd ; \
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
			mkdir -p ../out/$package ; \
			mkdir -p ../out/$package/$benchmark ; \
			mkdir -p ../out/$package/$benchmark/zdd ; \
			make $package'_'$benchmark'_zdd' ;
		done ;
done
'''

    suffix = f'''
echo -e "\\n========= Finished `date` ==========\\n"
'''

    return prefix + cudd_build + bdd_build + zdd_build + suffix

# =========================================================================== #
# Run Script Strings and Save to Disk
# =========================================================================== #
with open("build.sh", "w") as file:
    file.write(build_str())

for (t,b) in grouped_instances.items():
    for [filename, content] in benchmark_str(t,b):
        with open(filename, "w") as file:
            file.write(content)

print("\nScripts")
print("  Time Limits:      ", len(grouped_instances.keys()))
print("  Minimum Array:    ", min(map(lambda x : len(x[1]), grouped_instances.items())))
print("  Maximum Array:    ", max(map(lambda x : len(x[1]), grouped_instances.items())))
print("  Total Benchmarks: ", sum(map(lambda x : len(x[1]), grouped_instances.items())))
