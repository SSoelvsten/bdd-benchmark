import os

VARIANTS = ["adiar", "buddy", "cudd", "sylvan"]

# List of problems, with [N, Sylvan_M, max-time] triples.
#
# The sylvan memory are some 10 MiB/GiB above the actual computed value.
N_PROBLEMS = [

    ["queens",
     [[7,         128, "00-00:10"],
      [8,         128, "00-00:10"],
      [9,         128, "00-00:10"],
      [10,        128, "00-00:10"],
      [11,        512, "00-00:30"],
      [12,       1024, "00-01:00"],
      [13,   4 * 1024, "00-02:00"],
      [14,  32 * 1024, "00-02:30"],
      [15, 256 * 1024, "00-03:00"],
      [16, 330 * 1024, "00-20:00"],
      [17, 330 * 1024, "08-00:00"],
     ]
    ],

    ["sat_pigeonhole_principle",
     [[6,         128, "00-00:10"],
      [7,         128, "00-00:10"],
      [8,         128, "00-00:10"],
      [9,         140, "00-00:10"],
      [10,        140, "00-00:10"],
      [11,        140, "00-00:10"],
      [12,        280, "00-00:15"],
      [13,        550, "00-00:30"],
      [14,        550, "00-01:00"],
      [15,       1100, "00-06:00"],
     ]
    ],

    ["sat_queens",
     [[7,         128, "00-00:10"],
      [8,         128, "00-00:10"],
      [9,         300, "00-00:30"],
      [10,       1100, "00-03:00"],
      [11,  64 * 1024, "02-00:00"],
     ]
    ],

    ["tic_tac_toe",
     [[16,        128, "00-00:10"],
      [17,        128, "00-00:10"],
      [18,        128, "00-00:10"],
      [19,        140, "00-00:10"],
      [20,       1100, "00-00:10"],
      [21,   4 * 1024, "00-00:20"],
      [22,  32 * 1024, "00-01:00"],
      [23, 128 * 1024, "00-03:00"],
      [24, 256 * 1024, "00-12:00"],
      [25, 330 * 1024, "01-08:00"],
      [26, 330 * 1024, "04-00:00"],
     ]
    ],

]

def script_str_N(variant, problem_name, N, sylvan_M, time):
    output_dir = f'$SLURM_SUBMIT_DIR/out/{problem_name}/{variant}'
    output_file = f'{output_dir}/{N}.out'

    memory = sylvan_M if variant == "sylvan" else 300*1024

    return f'''#!/bin/bash
#SBATCH --job-name={variant}_{problem_name}_{N}
#SBATCH --partition=q48
#SBATCH --mem=350G
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time={time}:00
#SBATCH --exclusive
#SBATCH --mail-type=END
#SBATCH --mail-user=soelvsten@cs.au.dk

mkdir -p {output_dir}
touch {output_file}

echo -e "\\n=========  Started `date`  ==========\\n" | tee -a {output_file}

module load cmake gcc gpm
module load boost gmp
export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++
export LC_ALL=C

cd $SLURM_SUBMIT_DIR/build/src/
./{variant}_{problem_name} -N {N} -M {memory} -t /scratch/$SLURM_JOB_ID 2>&1 | tee -a {output_file}

echo -e "\\nexit code: "$? | tee -a {output_file}

echo -e "\\n========= Finished `date` ==========\\n" | tee -a {output_file}
'''


CIRCUIT_PROBLEMS = [

    # [ "name",       sylvan_M,   ["size",     "depth"],    "variable ordering"]
    ["arithmetic",
     [[ "adder",             140, ["00-00:10", "00-00:10"], "LEVEL_DFS" ],
      [ "bar",        330 * 1024, ["05-00:00", "00-10:00"], "LEVEL_DFS" ],
      [ "div",        330 * 1024, ["15-00:00", "15-00:00"], "LEVEL_DFS" ],
      [ "hyp",        330 * 1024, ["15-00:00", "04-00:00"], "LEVEL_DFS" ],
      [ "log2",       330 * 1024, ["15-00:00", "15-00:00"], "LEVEL_DFS" ],
      [ "max",        330 * 1024, ["15-00:00", "06-12:00"], "LEVEL_DFS" ],
      [ "multiplier", 330 * 1024, ["15-00:00", "15-00:00"], "LEVEL_DFS" ],
      [ "sin",          5 * 1024, ["00-03:00", "04-16:00"], "LEVEL_DFS" ],
      [ "sqrt",       330 * 1024, ["15-00:00", "15-00:00"], "LEVEL_DFS" ],
      [ "square",     330 * 1024, ["15-00:00", "15-00:00"], "LEVEL_DFS" ],
     ]
    ],

    ["random_control",
     [[ "arbiter",   256 * 1024, ["00-16:00", "06-00:00"], "INPUT" ],
      [ "cavlc",            140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "ctrl",             140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "dec",              140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "i2c",              140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "int2float",        140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "mem_ctrl",  128 * 1024, ["15-00:00", "15-00:00"], "INPUT" ],
      [ "priority",         140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "router",           140, ["00-00:10", "00-00:10"], "INPUT" ],
      [ "voter",      64 * 1024, ["00-04:00", "00-12:00"], "INPUT" ],
     ]
    ]
]

def script_str_picotrav(variant, problem_name, file_1, file_2, sylvan_M, time, variable_order):
    output_dir = f'$SLURM_SUBMIT_DIR/out/picotrav/{variant}'
    output_file = f'{output_dir}/{problem_name}.out'

    input_dir = f'$SLURM_SUBMIT_DIR/benchmarks/epfl'
    input_file_1 = f'{input_dir}/{file_1}'
    input_file_2 = f'{input_dir}/{file_2}'

    memory = sylvan_M if variant == "sylvan" else 300*1024

    return f'''#!/bin/bash
#SBATCH --job-name={variant}_{problem_name}
#SBATCH --partition=q48
#SBATCH --mem=350G
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time={time}:00
#SBATCH --exclusive
#SBATCH --mail-type=END
#SBATCH --mail-user=soelvsten@cs.au.dk

mkdir -p {output_dir}
touch {output_file}

echo -e "\\n=========  Started `date`  ==========\\n" | tee -a {output_file}

module load cmake gcc gpm
module load boost gmp
export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++
export LC_ALL=C

cd $SLURM_SUBMIT_DIR/build/src/
./{variant}_picotrav -f {input_file_1} -f {input_file_2} -M {memory} -o {variable_order} -t /scratch/$SLURM_JOB_ID 2>&1 | tee -a {output_file}

echo -e "\\nexit code: "$? | tee -a {output_file}

echo -e "\\n========= Finished `date` ==========\\n" | tee -a {output_file}
'''

for variant in VARIANTS:
    for problem in N_PROBLEMS:
        problem_name = problem[0]

        for instance in problem[1]:
            N = instance[0]
            sylvan_M = instance[1]
            time = instance[2]

            filename = f"{variant}_{problem_name}_{N}.sh"

            os.system(f"rm -f {filename} && touch {filename}")

            with open(filename, "w") as file:
                file.write(script_str_N(variant, problem_name, N, sylvan_M, time))

    for circuits_section in CIRCUIT_PROBLEMS:
        circuit_type = circuits_section[0]
        for instance_a in circuits_section[1]:
            for instance_b in ["size", "depth"]:
                problem_name = f"{instance_a[0]}_{instance_b}"

                file_1_base = [f for f in os.listdir(f"../benchmarks/epfl/best_results/{instance_b}") if f.startswith(instance_a[0])][0]
                file_1 = f"best_results/{instance_b}/{file_1_base}"
                file_2 = f"{circuit_type}/{instance_a[0]}.blif"
                sylvan_M = instance_a[1]
                time = instance_a[2][0 if instance_b == "size" else 1]
                variable_order = instance_a[3]

                filename = f"{variant}_{circuit_type}_{instance_a[0]}_{instance_b}.sh"

                os.system(f"rm -f {filename} && touch {filename}")

                with open(filename, "w") as file:
                    file.write(script_str_picotrav(variant, problem_name, file_1, file_2, sylvan_M, time, variable_order))
