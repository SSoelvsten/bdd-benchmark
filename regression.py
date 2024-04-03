# ---------------------------------------------------------------------------- #
#                 REGRESSION TESTING FOR CONTINUOUS INTEGRATION                #
# ---------------------------------------------------------------------------- #

yes_choices = ['yes', 'y']
no_choices = ['no', 'n']

# ---------------------------------------------------------------------------- #
import os, selectors, subprocess, sys

def run_subprocess(subprocess_cmds, subprocess_dir = ".", verbose = False):
    # Start subprocess
    # bufsize = 1 means output is line buffered
    # universal_newlines = True is required for line buffering
    process = subprocess.Popen(" && ".join(subprocess_cmds),
                               bufsize=1,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               universal_newlines=True,
                               shell=True,
                               cwd=f"{os.path.dirname(os.path.abspath(__file__))}/{subprocess_dir}")

    # Create callback function for process output
    buf = io.StringIO()

    while process.poll() is None:
        line = process.stdout.readline()
        buf.write(line)

        if verbose:
            print(line, end='')

    # Wait for 1s for everything to flush and check whether it failed
    fail = process.wait(1000) != 0

    last_lines = process.stdout.read()
    buf.write(last_lines)

    if verbose:
        print(last_lines, end='')

    # Store buffered output
    output = buf.getvalue()
    buf.close()

    return (fail, output)

# ---------------------------------------------------------------------------- #
# Pastva and Henzinger's BDDs (2023) for Apply

import os, io, zipfile, wget

class ApplyStrategy:
    _1_path = None
    _2_path = None

    def _tsv_path(self, folder):
        return f"{folder}/benchmarks.tsv"

    def _download_zenodo(self, path, zip_url, tsv_url):
        # create directory (if it does not exist)
        if not os.path.exists(path):
            os.makedirs(path)

        # .tsv file with pairs of BDDs
        tsv_file = wget.download(tsv_url)
        os.rename(tsv_file, tsv_path(path))
        print("")

        # .zip file with binary encoding of BDDs
        zip_file = wget.download(zip_url)
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            for zip_info in zip_ref.infolist():
                if zip_info.is_dir():
                    continue

                if not zip_info.filename.endswith(".bdd"):
                    continue

                print(zip_info)
                zip_info.filename = os.path.basename(zip_info.filename)
                print(zip_info)
                zip_ref.extract(zip_info, path)

        os.remove(zip_file)
        print("")

    def __init__(self):
        # TODO: Minimal .zip file with only the bare minimum of BDDs necessary
        #       for this benchmarking set. This would drastically improve the
        #       time needed to download.

        # BDDs of up to 1 Million BDD nodes. This .zip file is "only" 373 MiB in size.
        ZENODO_1M__PATH = "../pastva_1M_pruned"
        ZENODO_1M__ZIP  = "https://zenodo.org/records/7958052/files/bdd-1M-pruned.zip?download=1"
        ZENODO_1M__TSV  = "https://zenodo.org/records/7958052/files/benchmarks-1M.tsv?download=1"

        # BDDs of up to 10 Million BDD nodes. This .zip file is 4.4 GiB in size.
        ZENODO_10M__PATH = "../pastva_10M_pruned"
        ZENODO_10M__ZIP = "https://zenodo.org/records/7958052/files/bdd-10M-pruned.zip?download=1"
        ZENODO_10M__TSV = "https://zenodo.org/records/7958052/files/benchmarks-10M.tsv?download=1"

        inputs_folder = None

        # Download Data Set (if necessary)
        if not os.path.isdir(ZENODO_1M__PATH):
            print("Pastva & Henzinger (1M) not found.")
            if input("  | Download from Zenodo (373 MiB)? (yes/No): ").strip().lower() in yes_choices:
                self._download_zenodo(ZENODO_1M__PATH, ZENODO_1M__ZIP, ZENODO_1M__TSV)
                inputs_folder = ZENODO_1M__PATH
        else:
            print("Pastva & Henzinger (1M) found.")
            inputs_folder = ZENODO_1M__PATH

        if not os.path.isdir(ZENODO_10M__PATH):
            print("Pastva & Henzinger (10M) not found.")
            if input("  | Download from Zenodo (4.4 GiB)? (yes/No): ").strip().lower() in yes_choices:
                self._download_zenodo(ZENODO_10M__PATH, ZENODO_10M__ZIP, ZENODO_10M__TSV)
                inputs_folder = ZENODO_10M__PATH
        else:
            print("Pastva & Henzinger (10M) found.")
            inputs_folder = ZENODO_10M__PATH

        if not inputs_folder:
            print("")
            print("No folder containing inputs found. Aborting...")
            exit(255)

        print("")

        # Find largest requested inputs
        print("Upper bound on BDD Size")
        max_size = int(input(f"  | ").strip().lower())

        _1_size = 0
        _2_size = 0

        with open(self._tsv_path(inputs_folder), 'r') as tsv_file:
            tsv_content = tsv_file.read().splitlines()

            for [p1, p2] in [line.split('\t') for line in tsv_content]:
                s1 = int(p1.split('.')[0])
                s2 = int(p2.split('.')[0])

                if s1 < max_size and s2 < max_size:
                    if _1_size * _2_size < s1 * s2:
                        self._1_path = p1
                        _1_size = s1
                        self._2_path = p2
                        _2_size = s2

            self._1_path = f"{inputs_folder}/{self._1_path}"
            self._2_path = f"{inputs_folder}/{self._2_path}"

        print(f"  |")
        print(f"  | {self._1_path}\t({((2+2*4)*_1_size) / (1024*1024):.3f} MiB)")
        print(f"  | {self._2_path}\t({((2+2*4)*_2_size) / (1024*1024):.3f} MiB)")

        print("")

    def args(self):
        return f"-f {self._1_path} -f {self._2_path} -o AND"

    def report_name(self):
        return f"Apply({self._1_path.split('/')[-1]}, {self._2_path.split('/')[-1]})"

# ---------------------------------------------------------------------------- #
# EPFL Circuits for Picotrav

import os, io

class PicotravStrategy:
    _circuit = None

    _1_path = None
    _2_path = None

    def _download_git(self):
        (ret, _) = run_subprocess(["git clone https://github.com/lsils/benchmarks epfl"], "..", True)

        if ret != 0:
            print("Cloning of EPFL failed")
            exit(ret)

    def __init__(self):
        inputs_folder = False
        epfl_path = "../epfl"

        if not os.path.isdir(epfl_path):
            print("EPFL Benchmarks not found.")
            if input("  | Clone with Git? (yes/No): ").strip().lower() in yes_choices:
                self._download_git()
                inputs_folder = True
        else:
            print("EPFL Benchmarks found.")
            inputs_folder = True

        if not inputs_folder:
            print("")
            print("No folder containing inputs found. Aborting...")
            exit(255)

        print("")

        print("Circuit Name.")
        self._circuit = input("  | ").strip().lower()

        _1_base = [f"{epfl_path}/arithmetic", f"{epfl_path}/random_control"]

        _1_results = [[f"{path}/{f}" for f
                       in os.listdir(f"{path}")
                       if (f.startswith(self._circuit) and f.endswith(".blif"))]
                      for path in _1_base]

        _1_results = [f for fs in _1_results for f in fs]

        if not _1_results:
            print("")
            print("Specification Circuit not found. Aborting...")
            exit(255)

        self._1_path = _1_results[0]

        _2_base = f"{epfl_path}/best_results/depth"
        _2_results = [f"{_2_base}/{f}" for f in os.listdir(_2_base) if f.startswith(self._circuit)]

        if not _2_results:
            print("")
            print("Optimized Circuit not found. Aborting...")
            exit(255)

        self._2_path = _2_results[0]

        print(f"  |")
        print(f"  | {self._1_path}")
        print(f"  | {self._2_path}")

        print("")

    def args(self):
        return f"-f {self._1_path} -f {self._2_path} -o level_df"

    def report_name(self):
        return f"Picotrav '{self._circuit}'"

# ---------------------------------------------------------------------------- #
# QCIR Circuits for QBF

import os, io, zipfile, wget

class QbfStrategy:
    _name = None
    _path = None

    def _download_zip(self, path, zip_url):
        # download and extract .zip file
        zip_file = wget.download(zip_url)

        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            zip_ref.extractall(".")

        # move zip file to the desired destination
        folder_name = zip_file[:-4]
        os.rename(folder_name, path)

        # remove .zip file
        os.remove(zip_file)
        print("")

    def __init__(self):
        inputs_folder = False

        SAT2023_GDDL_PATH = "../SAT2023_GDDL"
        SAT2023_GDDL_URL  = "https://github.com/irfansha/Q-sage/raw/main/Benchmarks/SAT2023_GDDL.zip"

        if not os.path.isdir(SAT2023_GDDL_PATH):
            print("GDDL SAT 2023 Benchmarks not found.")
            if input("  | Download from 'github.com/irfansha/q-sage'? (yes/No): ").strip().lower() in yes_choices:
                self._download_zip(SAT2023_GDDL_PATH, SAT2023_GDDL_URL)
                inputs_folder = True
        else:
            print("GDDL SAT 2023 Benchmarks found.")
            inputs_folder = True

        if not inputs_folder:
            print("")
            print("No folder containing inputs found. Aborting...")
            exit(255)

        print("")

        self._path = f"{SAT2023_GDDL_PATH}/QBF_instances"

        SAT2023_GDDL_CATEGORIES = Enum('categories_t', ['breakthrough', 'breakthrough_dual', 'connect4', 'domineering', 'ep', 'ep_dual', 'hex', 'httt'])

        print(f"Category ({', '.join(SAT2023_GDDL_CATEGORIES._member_names_)})")
        category = SAT2023_GDDL_CATEGORIES.breakthrough

        category_input = input(f"  | ").strip().lower()
        if category_input in SAT2023_GDDL_CATEGORIES._member_names_:
            category = [c for c in SAT2023_GDDL_CATEGORIES if c.name == category_input][0]
        else:
            print(f"\nUnknown Category '{benchmark_input}'. Aborting...")
            exit(255)

        match category:
            case SAT2023_GDDL_CATEGORIES.breakthrough:
                self._path += "/B"
            case SAT2023_GDDL_CATEGORIES.breakthrough_dual:
                self._path += "/BSP"
            case SAT2023_GDDL_CATEGORIES.connect4:
                self._path += "/C4"
            case SAT2023_GDDL_CATEGORIES.domineering:
                self._path += "/D"
            case SAT2023_GDDL_CATEGORIES.ep:
                self._path += "/EP"
            case SAT2023_GDDL_CATEGORIES.ep_dual:
                self._path += "/EP-dual"
            case SAT2023_GDDL_CATEGORIES.hex:
                self._path += "/hex"
            case SAT2023_GDDL_CATEGORIES.httt:
                self._path += "/httt"

        print("")

        instances = [f[:-5] for f in os.listdir(self._path) if f.endswith('.qcir')]

        print("Instances")
        print(f"  | {', '.join(instances)}")
        print("  |")
        circuit = input("  | ").strip().lower()

        if circuit not in instances:
            print(f"\nUnknown Circuit '{circuit}'. Aborting...")
            exit(255)

        self._path += f"/{circuit}.qcir"

        print(f"  |")
        print(f"  | {self._path}")

        self._name = f"{category.name}/{circuit}"

        print("")

    def args(self):
        return f"-f {self._path} -o df"

    def report_name(self):
        return f"QBF '{self._name}'"

# ---------------------------------------------------------------------------- #
# Queens

class QueenStrategy:
    N = 8

    def __init__(self):
        print("Queens Board Size")
        self.N = int(input("  | ").strip().lower())

        if self.N < 1 or self.N > 16:
            print(f"  | Value '{self.N}' is out of range. Aborting...")
            exit(255)

        print("")

    def args(self):
        return f"-N {self.N}"

    def report_name(self):
        return f"{self.N}-Queens"

# ---------------------------------------------------------------------------- #
# Choice of Benchmark

from enum import Enum

Benchmarks = Enum('benchmark_t', ['apply', 'picotrav', 'qbf', 'queens'])

benchmark = Benchmarks.apply

print(f"Benchmark ({', '.join(Benchmarks._member_names_)})")
benchmark_input = input(f"  | ").strip().lower()
if benchmark_input in Benchmarks._member_names_:
    benchmark = [b for b in Benchmarks if b.name == benchmark_input][0]
else:
    print(f"\nUnknown Benchmark '{benchmark_input}'. Aborting...")
    exit(255)

print("")

match benchmark:
    case Benchmarks.apply:
        benchmark_strategy = ApplyStrategy()
    case Benchmarks.picotrav:
        benchmark_strategy = PicotravStrategy()
    case Benchmarks.qbf:
        benchmark_strategy = QbfStrategy()
    case Benchmarks.queens:
        benchmark_strategy = QueenStrategy()

# ---------------------------------------------------------------------------- #
# Choice of BDD package to test

from enum import Enum

Packages = Enum('package_t', ['adiar', 'buddy', 'cal', 'cudd', 'sylvan'])

package = Packages.adiar

print(f"BDD Package ({', '.join(Packages._member_names_)})")
package_input = input(f"  | ").strip().lower()
if package_input in Packages._member_names_:
    package = [p for p in Packages if p.name == package_input][0]
else:
    print(f"\nUnknown BDD package '{package_input}'. Aborting...")
    exit(255)

submodule_path = f"./external/{package.name}"

print("\nInternal Memory (MiB)")
package_memory = int(input(f"  | ").strip().lower())
print("")

# ---------------------------------------------------------------------------- #
# Git Remote and Branch

import os

print(f"Git: Baseline     (e.g. main)")
baseline_remote = input(f"  | remote: ").strip().lower()
baseline_branch = input(f"  | branch: ").strip().lower()

print(f"Git: To-Be Tested (e.g. pull request branch)")
test_remote = input(f"  | remote: ").strip().lower()
test_branch = input(f"  | branch: ").strip().lower()

def git__mangle(remote):
    if remote == 'origin':
        return remote

    return f"tmp__{remote.replace('/','_')}"

def git__remove(remote):
    if (remote == 'origin'):
        return # origin should not be purged

    return run_subprocess([f"git remote remove {git__mangle(remote)}"], submodule_path)

def git__add(remote):
    if (remote == 'origin'):
        return # origin hopefully exists

    (remove_ret, remove_output) = git__remove(remote)

    (add_ret, add_output) = run_subprocess([f"git remote add {git__mangle(remote)} https://github.com/{remote}.git"],
                                           submodule_path)

    return (remove_ret or add_ret, f"{remove_output}\n\n{add_output}")

def git__fetch():
    return run_subprocess(["git fetch --all"], submodule_path)

def git__checkout(remote, branch):
    return run_subprocess([f"git checkout {git__mangle(remote)}/{branch}",
                           "git submodule update --init --recursive",
                           "git status"],
                          submodule_path)

print("")

# ---------------------------------------------------------------------------- #
# Build and Run Instance(s)

import re

print(f"Verbose Output?")

print_build = False
if input("  | Build (yes/No): ").strip().lower() in yes_choices:
    print_build = True

print_benchmark = False
if input("  | Benchmark (yes/No): ").strip().lower() in yes_choices:
    print_benchmark = True

def build(remote, branch):
    if package == Packages.cudd:
        print(f"  | {package.cudd.name} not (yet) supported. Aborting...")
        exit(255)

    if print_build:
        print("")

    (build_fail, build_output) = run_subprocess(["cmake -E make_directory ./build",
                                                 "cd build",
                                                 f"cmake ../ -D CMAKE_BUILD_TYPE=Release",
                                                 f"cmake --build . --target {package.name}_{benchmark.name}_bdd"],
                                                ".",
                                                print_build)

    if not print_build:
        if build_fail:
            print("  |  |  | Failed! Aborting...")
            print(f"\n{build_output}")

            exit(255)

def run(remote, branch):
    if print_benchmark:
        print("")

    exec_name = f"./build/src/{package.name}_{benchmark.name}_bdd"
    args = f"-M {package_memory} {benchmark_strategy.args()}"

    (run_fail, run_output) = run_subprocess([f"{exec_name} {args}"], ".", print_benchmark)

    time = int(re.findall(r".*\s*total time.*\s+([0-9\.]+)\s*", run_output)[0])

    if not print_benchmark:
        if run_fail:
            print("  |  |  | Benchmark Failed. Aborting...")
            print(f"\n{run_output}")

            exit(255)

    return time

print("")

# ---------------------------------------------------------------------------- #
# Run experiment

import time, statistics

class Data:
    _raw = None

    # Add a new sample
    def add(self, time):
        self._raw.append(time)

    # Number of collected samples
    def samples(self):
        return max(len(self._raw)-1, 0)

    # Average of all samples
    def mean(self):
        if len(self._raw) <= 1:
            return float('nan')

        return statistics.mean(self._raw[1:])

    # stdev
    def stdev(self):
        return statistics.stdev(self._raw[1:])

    # Normalalised stdev
    def stdev_norm(self):
        if len(self._raw) <= 2:
            return 1

        return self.stdev() / self.mean()

    # Constructor
    def __init__(self):
        self._raw = []

baseline_data = Data()
test_data = Data()

git__add(baseline_remote)
git__add(test_remote)

git__fetch()

epsilon = 0.05

start_time   = time.time()
min_end_time = start_time + (60*30) # 30 minutes
max_end_time = start_time + (60*90) # 90 minutes

print("Collecting Samples:")
min_samples = int(input("  | min: "))
max_samples = int(input("  | max: "))
print("  |")

def sample(data, remote, branch):
    print(f"  | {remote}/{branch}:")

    print(f"  |  | Git Checkout...")
    git__checkout(remote, branch)

    print(f"  |  | Building...")
    build(remote, branch)
    print("  |  |  | Done!")

    print(f"  |  | Running Benchmark...")
    time = run(remote, branch)
    data.add(time)
    print(f"  |  |  | Time: {time}")

while ( # Run for at least 'min_samples'
        test_data.samples() < min_samples
       ) or (
           ( # If we have not exhausted the number of samples
               (test_data.samples() < max_samples)
           ) and (
               # The noise is too high or we have plenty of time left
               baseline_data.stdev_norm() > epsilon or test_data.stdev_norm() > epsilon or time.time() < min_end_time
           ) and (
               # But we have not spent too much time already
               time.time() < max_end_time
           )
       ):
    assert(test_data.samples() == baseline_data.samples())

    print(f"  | ----------------------------------------------------")
    sample(baseline_data, baseline_remote, baseline_branch)
    sample(test_data, test_remote, test_branch)

    print(f"  |")
    print(f"  | Baseline: {baseline_data.mean():.2f} ({(baseline_data.stdev_norm() * 100):.2f}%)")
    print(f"  | Test:     {test_data.mean():.2f} ({(test_data.stdev_norm() * 100):.2f}%)")

print("")

# ---------------------------------------------------------------------------- #
# Write report

def markdown_table(column_headers, row_headers, data):
    assert(len(data) == len(column_headers))
    for d in data:
        assert(len(d) == len(row_headers))

    header = f"| ... | {' | '.join(column_headers)} |"
    sep    = f"|-----|-{'-|-'.join(re.sub(r'.','-',h) for h in column_headers)}-|"
    rows   = []

    for i in range(len(data)):
        rows.append(f"| {row_headers[i]} | {' | '.join([f'{d:.2f}' for d in data[i]])} |")

    nl = '\n'
    return f"{header}{nl}{sep}{nl}{nl.join(rows)}"

exit_code = 0

with open(f"regression_{package.name}.out", 'a') as out:
    max_stdev = max(baseline_data.stdev(), test_data.stdev())
    diff = baseline_data.mean() - test_data.mean()
    significant = abs(diff) > max_stdev

    if diff < 0 and significant:
        exit_code = 1

    color = ('red' if significant else 'yellow') if diff < 0 else 'green'
    headline_txt   = ("# "
                      + f":{color}_circle:"
                      + f" Regression Test ({benchmark_strategy.report_name()})")

    conclusion_txt = (f"'{test_remote}/{test_branch}'"
                      + f" is a change in performance of {diff/baseline_data.mean()*100:.2f}%"
                      + f" (stdev: {max(baseline_data.stdev_norm(), test_data.stdev_norm())*100:.2f}%).")

    summary_table  = markdown_table([f"{baseline_remote}/{baseline_branch}", f"{test_remote}/{test_branch}"],
                                    ["Mean", "Standard Deviation"],
                                    [[baseline_data.mean(), test_data.mean()],[baseline_data.stdev(), test_data.stdev()]])

    samples_txt    = (f"Number of samples: {baseline_data.samples()}")

    out.write(f"{headline_txt}\n{conclusion_txt}\n\n{summary_table}\n\n{samples_txt}")

print("Outputting Report")
print(f"  | {test_remote}/{test_branch} is {'good' if exit_code == 0 else 'bad'}")

exit(exit_code)
