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
# Petri Nets and Boolean Networks for McNet

import os, io, re, tarfile, wget

class McNetStrategy:
    _paths = []
    _path = ".."

    MCC_INSTANCES = [
        ["Anderson",           2023, "https://mcc.lip6.fr/2024/archives/Anderson-pnml.tar.gz"],
        ["EisenbergMcGuire",   2023, "https://mcc.lip6.fr/2024/archives/EisenbergMcGuire-pnml.tar.gz"],
        ["AutonomousCar",      2022, "https://mcc.lip6.fr/2024/archives/AutonomousCar-pnml.tar.gz"],
        ["RERS2020",           2022, "https://mcc.lip6.fr/2024/archives/RERS2020-pnml.tar.gz"],
        ["StigmergyCommit",    2022, "https://mcc.lip6.fr/2024/archives/StigmergyCommit-pnml.tar.gz"],
        ["StigmergyElection",  2022, "https://mcc.lip6.fr/2024/archives/StigmergyElection-pnml.tar.gz"],
        ["GPUForwardProgress", 2021, "https://mcc.lip6.fr/2024/archives/GPUForwardProgress-pnml.tar.gz"],
        ["HealthRecord",       2021, "https://mcc.lip6.fr/2024/archives/HealthRecord-pnml.tar.gz"],
        ["ServersAndClients",  2021, "https://mcc.lip6.fr/2024/archives/ServersAndClients-pnml.tar.gz"],
        ["ShieldIIPs",         2020, "https://mcc.lip6.fr/2024/archives/ShieldIIPs-pnml.tar.gz"],
        ["ShieldIIPt",         2020, "https://mcc.lip6.fr/2024/archives/ShieldIIPt-pnml.tar.gz"],
        ["ShieldPPPs",         2020, "https://mcc.lip6.fr/2024/archives/ShieldPPPs-pnml.tar.gz"],
        ["ShieldPPPt",         2020, "https://mcc.lip6.fr/2024/archives/ShieldPPPt-pnml.tar.gz"],
        ["SmartHome",          2020, "https://mcc.lip6.fr/2024/archives/SmartHome-pnml.tar.gz"],
        ["NoC3x3",             2019, "https://mcc.lip6.fr/2024/archives/NoC3x3-pnml.tar.gz"],
        ["ASLink",             2018, "https://mcc.lip6.fr/2024/archives/ASLink-pnml.tar.gz"],
        ["BusinessProcesses",  2018, "https://mcc.lip6.fr/2024/archives/BusinessProcesses-pnml.tar.gz"],
        ["DLCflexbar",         2018, "https://mcc.lip6.fr/2024/archives/DLCflexbar-pnml.tar.gz"],
        ["DiscoveryGPU",       2018, "https://mcc.lip6.fr/2024/archives/DiscoveryGPU-pnml.tar.gz"],
        ["EGFr",               2018, "https://mcc.lip6.fr/2024/archives/EGFr-pnml.tar.gz"],
        ["MAPKbis",            2018, "https://mcc.lip6.fr/2024/archives/MAPKbis-pnml.tar.gz"],
        ["NQueens",            2018, "https://mcc.lip6.fr/2024/archives/NQueens-pnml.tar.gz"],
    ]

    MCC_KEY = "mcc"
    MCC_PATH = f"{_path}/{MCC_KEY}"

    def _mcc_path(self, mcc_instance):
        return f"{self.MCC_KEY}/{mcc_instance[1]}/{mcc_instance[0]}"

    def _mcc_download(self, path, tar_url):
        # create directory (if it does not exist)
        if not os.path.exists(path):
            os.makedirs(path)

        # .tar file with binary encoding of BDDs
        tar_file = wget.download(tar_url)
        with tarfile.open(tar_file, 'r') as tar_ref:
            for tar_info in tar_ref:
                if tar_info.isdir() or re.search(".*/PT/.*", tar_info.name) == None:
                    continue

                tar_info.name = os.path.basename(tar_info.name)
                print(f"  {tar_info.name}")
                tar_ref.extract(tar_info, path)

        os.remove(tar_file)
        print("")

    AEON_KEY = "aeon"

    def _aeon_download(self, path):
        aeon_url = "https://github.com/sybila/biodivine-lib-param-bn/archive/refs/heads/master.zip"

        zip_file = wget.download(aeon_url)
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            for zip_info in zip_ref.infolist():
                # Ignore folders, anything not in 'sbml_models/real_world' or ending with '.aeon'
                if zip_info.is_dir():
                    continue
                if not re.search(".*/sbml_models/real_world/.*/.*", zip_info.filename):
                    continue
                if not zip_info.filename.endswith(".aeon"):
                    continue

                zip_info.filename = re.search(".*/sbml_models/real_world/(.*)/.*", zip_info.filename).group(1) + ".aeon"
                print(f"  {zip_info.filename}")
                zip_ref.extract(zip_info, path)

        os.remove(zip_file)
        print("")

    BNET_KEY = "bnet"

    def _bnet_download(self, path):
        bnet_url = "https://github.com/hklarner/pyboolnet/archive/refs/heads/master.zip"

        zip_file = wget.download(bnet_url)
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            for zip_info in zip_ref.infolist():
                # Ignore folders, anything not in 'pyboolnet/repository' or ending with '.bnet'
                if zip_info.is_dir():
                    continue
                if not re.search(".*/pyboolnet/repository/.*/.*", zip_info.filename):
                    continue
                if not zip_info.filename.endswith(".bnet"):
                    continue

                zip_info.filename = re.search(".*/pyboolnet/repository/.*/(.*)", zip_info.filename).group(1)
                print(f"  {zip_info.filename}")
                zip_ref.extract(zip_info, path)

        os.remove(zip_file)
        print("")

    def __init__(self):
        # Download MCC Competition
        for mcc_i in self.MCC_INSTANCES:
            path = f"../{self._mcc_path(mcc_i)}"
            if not os.path.isdir(path):
                print(f"MCC {mcc_i[0]} ({mcc_i[1]}) not found.")
                if input("  | Download? (yes/No): ").strip().lower() in yes_choices:
                    self._mcc_download(path, mcc_i[2])
                    self._paths.append(self._mcc_path(mcc_i))
            else:
                self._paths.append(self._mcc_path(mcc_i))

        # Download Boolean Networks (AEON)
        if not os.path.isdir(f"{self._path}/{self.AEON_KEY}"):
            print(f"AEON Models not found.")
            if input("  | Download? (yes/No): ").strip().lower() in yes_choices:
                self._aeon_download(f"{self._path}/{self.AEON_KEY}")
                self._paths.append(self.AEON_KEY)
        else:
            self._paths.append(self.AEON_KEY)

        # Download Boolean Networks (BNET)
        if not os.path.isdir(f"{self._path}/{self.BNET_KEY}"):
            print(f"BNET Models not found.")
            if input("  | Download? (yes/No): ").strip().lower() in yes_choices:
                self._bnet_download(f"{self._path}/{self.BNET_KEY}")
                self._paths.append(self.BNET_KEY)
        else:
            self._paths.append(self.BNET_KEY)

        # Choose folder
        print(f"Folder")
        for f in self._paths:
            print(f"  | {f}")
        print(f"  |")

        folder = None

        folder_input = input(f"  | ").strip().lower()
        if folder_input in [f.lower() for f in self._paths]:
            self._path = f"{self._path}/{[f for f in self._paths if f.lower() == folder_input][0]}"
        else:
            print(f"\nUnknown Folder '{folder_input}'. Aborting...")
            exit(255)

        print("")

        # Choose input in folder
        all_files = [f for f in os.listdir(self._path) if os.path.isfile(f"{self._path}/{f}")]
        print(f"File")
        for f in all_files:
            print(f"  | {f}")
        print(f"  |")

        file = None

        file_input = input(f"  | ").strip().lower()
        if file_input in [f.lower() for f in all_files]:
            self._path = f"{self._path}/{[f for f in all_files if f.lower() == file_input][0]}"
        else:
            print(f"\nUnknown File '{file_input}'. Aborting...")
            exit(255)

        print("")

    def args(self):
        return f"-f {self._path} -o sloan -a reach -a dead"

    def report_name(self):
        return f"McNet '{self._path[3:]}'"

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

Benchmarks = Enum('benchmark_t', ['apply', 'mcnet', 'picotrav', 'qbf', 'queens'])

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
    case Benchmarks.mcnet:
        benchmark_strategy = McNetStrategy()
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
    significant = abs(diff) > 2 * max_stdev

    if diff < 0 and significant:
        exit_code = 1

    color = ('red' if diff < 0 else 'green') if significant else 'yellow'
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
