import os

VARIANTS = ["adiar", "buddy", "sylvan"]

# List of problems, with [N, M, max-time] triples
PROBLEMS = [

    ["queens",
     [[7,         128, "00:10"],
      [8,         128, "00:10"],
      [9,         128, "00:10"],
      [10,        128, "00:10"],
      [11,        512, "00:30"],
      [12,       1024, "01:00"],
      [13,   4 * 1024, "02:00"],
      [14,  32 * 1024, "02:30"],
      [15, 256 * 1024, "03:00"],
      [16, 256 * 1024, "20:00"],
      [17, 256 * 1024, "8-00:00"],
     ]
    ],

    ["sat_pigeonhole_principle",
     [[6,         128, "00:10"],
      [7,         128, "00:10"],
      [8,         128, "00:10"],
      [9,         256, "00:10"],
      [10,        256, "00:10"],
      [11,        256, "00:10"],
      [12,        512, "00:15"],
      [13,       1024, "00:30"],
      [14,       1024, "01:00"],
      [15,   2 * 1024, "06:00"],
     ]
    ],

    ["sat_queens",
     [[7,         128, "00:10"],
      [8,         128, "00:10"],
      [9,         512, "00:30"],
      [10,   2 * 1024, "03:00"],
      [11,  64 * 1024, "48:00"],
     ]
    ],

    ["tic_tac_toe",
     [[16,        128, "00:10"],
      [17,        128, "00:10"],
      [18,        128, "00:10"],
      [19,        256, "00:10"],
      [20,   2 * 1024, "00:10"],
      [21,   4 * 1024, "00:20"],
      [22,  32 * 1024, "01:00"],
      [23, 128 * 1024, "03:00"],
      [24, 256 * 1024, "12:00"],
      [25, 256 * 1024, "32:00"],
      [26, 256 * 1024, "96:00"],
     ]
    ],

]

def script_str(variant, problem_name, N, M, time):
    output_dir = f'$SLURM_SUBMIT_DIR/out/{problem_name}'
    output_file = f'{output_dir}/{variant}_{N}.out'
    return f'''#!/bin/bash
#SBATCH --job-name={variant}_{problem_name}_{N}
#SBATCH --partition=q48
#SBATCH --mem=300G
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time={time}:00
#SBATCH --exclusive
#SBATCH --mail-type=END
#SBATCH --mail-user=soelvsten@cs.au.dk

mkdir -p {output_dir}
touch {output_file}

echo "\n=========  Started `date`  ==========\n" | tee -a {output_file}

lscpu | tee -a {output_file}
echo "\n"

module load cmake gcc gpm boost gmp
export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++
export LC_ALL=C

cd $SLURM_SUBMIT_DIR/build/src/
./{variant}_{problem_name} {N} {M} /scratch/$SLURM_JOB_ID 2>&1 | tee -a {output_file}

echo "\n========= Finished `date` ==========\n" | tee -a {output_file}
'''

for variant in VARIANTS:
    for problem in PROBLEMS:
        problem_name = problem[0]

        for instance in problem[1]:
            N = instance[0]
            M = instance[1]
            time = instance[2]

            filename = f"{variant}_{problem_name}_{N}.sh"

            os.system(f"rm -f {filename} && touch {filename}")

            with open(filename, "w") as file:
                file.write(script_str(variant, problem_name, N, M, time))
