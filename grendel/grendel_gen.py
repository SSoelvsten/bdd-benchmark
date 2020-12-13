import os

VARIANTS = ["buddy", "coom", "sylvan"]

# List of problems, with [N,max-time] tuples
PROBLEMS = [

    ["queens",
     [[7,  "00:10"],
      [8,  "00:10"],
      [9,  "00:10"],
      [10, "00:10"],
      [11, "00:30"],
      [12, "01:00"],
      [13, "02:00"],
      [14, "02:30"],
      [15, "03:00"],
      [16, "48:00"],
     ]
    ],

    ["sat_pigeonhole_principle",
     [[6,  "00:10"],
      [7,  "00:10"],
      [8,  "00:10"],
      [9,  "00:10"],
      [10, "00:10"],
      [11, "00:10"],
      [12, "00:15"],
      [13, "00:30"],
      [14, "01:00"],
      [15, "06:00"],
     ]
    ],

    ["sat_queens",
     [[7,  "00:10"],
      [8,  "00:10"],
      [9,  "00:30"],
      [10, "03:30"],
     ]
    ],

    ["tic_tac_toe",
     [[16, "00:10"],
      [17, "00:10"],
      [18, "00:10"],
      [19, "00:10"],
      [20, "00:10"],
      [21, "00:20"],
      [22, "01:00"],
      [23, "03:00"],
      [24, "12:00"],
      [25, "48:00"],
     ]
    ],

]

def script_str(variant, problem_name, N, time):
    output_dir = f'$SLURM_SUBMIT_DIR/out/{problem_name}'
    output_file = f'{output_dir}/{variant}_{N}.out'
    return f'''#!/bin/bash
#SBATCH --job-name={variant}_{problem_name}_{N}
#SBATCH --partition=q48
#SBATCH --mem=156G
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time={time}:00
#SBATCH --exclusive

mkdir -p {output_dir}
touch {output_file}

echo "\n=========  Started `date`  ==========\n" | tee -a {output_file}

module load cmake gcc gpm boost gmp
export CC=/comm/swstack/core/gcc/10.1.0/bin/gcc
export CXX=/comm/swstack/core/gcc/10.1.0/bin/c++
export LC_ALL=C

cd $SLURM_SUBMIT_DIR/build/src/
./{variant}_{problem_name} {N} 126976 /scratch/$SLURM_JOB_ID 2>&1 | tee -a {output_file}

echo "\n========= Finished `date` ==========\n" | tee -a {output_file}
'''

for variant in VARIANTS:
    for problem in PROBLEMS:
        problem_name = problem[0]

        for instance in problem[1]:
            N = instance[0]
            time = instance[1]

            filename = f"{variant}_{problem_name}_{N}.sh"

            os.system(f"rm -f {filename} && touch {filename}")

            with open(filename, "w") as file:
                file.write(script_str(variant, problem_name, N, time))
