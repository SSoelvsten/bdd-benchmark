# =========================================================================== #
# User Inputs
# =========================================================================== #
yes_choices = ['yes', 'y']
no_choices = ['no', 'n']

# =========================================================================== #
# BDD Packages and their supported Diagrams.
# =========================================================================== #
from enum import Enum

dd_t = Enum('dd_t', ['bdd', 'bcdd', 'zdd'])

dd_choice = []
for dd in dd_t:
    if input(f"Include '{dd.name.upper()}' benchmarks? (yes/No): ").lower() in yes_choices:
        dd_choice.append(dd)

if not dd_choice:
    print("\n  At least one kind of Decision Diagram should be included!")
    exit(1)

package_t = Enum('package_t', ['adiar', 'buddy', 'cal', 'cudd', 'libbdd', 'oxidd', 'sylvan'])

package_dd = {
    package_t.adiar  : [dd_t.bdd, dd_t.zdd],
    package_t.buddy  : [dd_t.bdd],
    package_t.cal    : [dd_t.bcdd],
    package_t.cudd   : [dd_t.bcdd, dd_t.zdd],
    package_t.libbdd : [dd_t.bdd],
    package_t.oxidd  : [dd_t.bdd, dd_t.bcdd, dd_t.zdd],
    package_t.sylvan : [dd_t.bcdd]
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

bdd_packages = []
if dd_t.bdd in dd_choice:
    bdd_packages = [p for p in package_choice if dd_t.bdd in package_dd[p] or dd_t.bcdd in package_dd[p]]

zdd_packages = []
if dd_t.zdd in dd_choice:
    zdd_packages = [p for p in package_choice if dd_t.zdd in package_dd[p]]

print("\nPackages")
print("  BDD:", [p.name for p in bdd_packages])
print("  ZDD:", [p.name for p in zdd_packages])

# =========================================================================== #
# Benchmark Instances
# =========================================================================== #

# --------------------------------------------------------------------------- #
def mcnet__args(path, merge):
    assert(os.path.exists(f"../../{path}"))
    return f"-f ../{path} -a reach -a dead -a scc -o sloan{' -s async' if merge else ''}"

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

# --------------------------------------------------------------------------- #
def qbf__args(circuit_name):
    # All of Irfansha Shaik's circuits seem to be output in a depth-first order.
    return f"-o df -f ../SAT2023_GDDL/QBF_instances/{circuit_name}.qcir"

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
            [ [ 0, 0,10], "-s none -n 3 -n 3" ],
            [ [ 0, 0,10], "-s none -n 4 -n 3" ],
            [ [ 0, 0,10], "-s none -n 4 -n 4" ],
            [ [ 0, 0,10], "-s none -n 5 -n 4" ],
            [ [ 0, 0,30], "-s none -n 5 -n 5" ],
            [ [ 0, 2,00], "-s none -n 6 -n 5" ],
            [ [ 2, 0,00], "-s none -n 6 -n 6" ],
            [ [15, 0,00], "-s none -n 7 -n 6" ],
            # Mirror Vertical
            [ [ 0, 0,10], "-s mirror-vertical -n 3 -n 3" ],
            [ [ 0, 0,10], "-s mirror-vertical -n 4 -n 3" ],
            [ [ 0, 0,10], "-s mirror-vertical -n 4 -n 4" ],
            [ [ 0, 0,10], "-s mirror-vertical -n 5 -n 4" ],
            [ [ 0, 0,10], "-s mirror-vertical -n 5 -n 5" ],
            [ [ 0, 0,30], "-s mirror-vertical -n 6 -n 5" ],
            [ [ 0, 4,00], "-s mirror-vertical -n 6 -n 6" ],
            [ [ 0,12,00], "-s mirror-vertical -n 7 -n 6" ],
            [ [ 1, 0,00], "-s mirror-vertical -n 7 -n 7" ],
            [ [ 4, 0,00], "-s mirror-vertical -n 8 -n 7" ],
            # Mirror Quadrant
            [ [ 0, 0,10], "-s mirror-quad -n 3 -n 3" ],
            [ [ 0, 0,10], "-s mirror-quad -n 4 -n 3" ],
            [ [ 0, 0,10], "-s mirror-quad -n 4 -n 4" ],
            [ [ 0, 0,10], "-s mirror-quad -n 5 -n 4" ],
            [ [ 0, 0,10], "-s mirror-quad -n 5 -n 5" ],
            [ [ 0, 0,10], "-s mirror-quad -n 6 -n 5" ],
            [ [ 0, 0,30], "-s mirror-quad -n 6 -n 6" ],
            [ [ 0,12,00], "-s mirror-quad -n 7 -n 6" ],
            [ [ 1, 0,00], "-s mirror-quad -n 7 -n 7" ],
            [ [ 4, 0,00], "-s mirror-quad -n 8 -n 7" ],
            # Mirror Diagonal
            [ [ 0, 0,10], "-s mirror-diagonal -n 3 -n 3" ],
            [ [ 0, 0,10], "-s mirror-diagonal -n 4 -n 4" ],
            [ [ 0, 0,30], "-s mirror-diagonal -n 5 -n 5" ],
            [ [ 1, 0,00], "-s mirror-diagonal -n 6 -n 6" ],
            [ [ 2, 0,00], "-s mirror-diagonal -n 7 -n 7" ],
            # Mirror Double Diagonal
            [ [ 0, 0,10], "-s mirror-double_diagonal -n 3 -n 3" ],
            [ [ 0, 0,10], "-s mirror-double_diagonal -n 4 -n 4" ],
            [ [ 0, 0,10], "-s mirror-double_diagonal -n 5 -n 5" ],
            [ [ 0, 3,00], "-s mirror-double_diagonal -n 6 -n 6" ],
            # Rotate 90
            [ [ 0, 0,10], "-s rotate-90 -n 3 -n 3" ],
            [ [ 0, 0,10], "-s rotate-90 -n 4 -n 4" ],
            [ [ 0, 0,10], "-s rotate-90 -n 5 -n 5" ],
            [ [ 0, 0,30], "-s rotate-90 -n 6 -n 6" ],
            # Rotate 180
            [ [ 0, 0,10], "-s rotate-180 -n 3 -n 3" ],
            [ [ 0, 0,10], "-s rotate-180 -n 4 -n 3" ],
            [ [ 0, 0,10], "-s rotate-180 -n 4 -n 4" ],
            [ [ 0, 0,10], "-s rotate-180 -n 5 -n 4" ],
            [ [ 0, 0,30], "-s rotate-180 -n 5 -n 5" ],
            [ [ 0, 1,00], "-s rotate-180 -n 6 -n 5" ],
            [ [ 2, 0,00], "-s rotate-180 -n 6 -n 6" ],
            [ [15, 0,00], "-s rotate-180 -n 7 -n 6" ],
        ]
    },
    # --------------------------------------------------------------------------
    "hamiltonian": {
        dd_t.bdd: [
            # Binary Encoding
            [ [ 0, 0,10], "-e binary -n 4 -n 3" ],
            [ [ 0, 0,10], "-e binary -n 4 -n 4" ],
            [ [ 0, 0,10], "-e binary -n 5 -n 4" ],
            [ [ 0, 0,30], "-e binary -n 6 -n 5" ],
            [ [ 0, 1,00], "-e binary -n 6 -n 6" ],
            [ [ 0, 2,00], "-e binary -n 7 -n 6" ],
            [ [ 0,12,00], "-e binary -n 8 -n 6" ],
            [ [ 2, 0,00], "-e binary -n 8 -n 7" ],
          # [ [15, 0,00], "-e binary -n 8 -n 8" ],
        ],
        dd_t.zdd: [
            # Time-based Encoding
            [ [ 0, 0,10], "-e time -n 4 -n 3" ],
            [ [ 0, 0,10], "-e time -n 4 -n 4" ],
            [ [ 0, 0,10], "-e time -n 5 -n 4" ],
            [ [ 0, 0,10], "-e time -n 6 -n 5" ],
            [ [ 0, 0,10], "-e time -n 6 -n 6" ],
            [ [ 0, 0,30], "-e time -n 7 -n 6" ],
            [ [ 0, 1,00], "-e time -n 8 -n 6" ],
            [ [ 0, 1,00], "-e time -n 8 -n 7" ],
          # [ [15, 0,00], "-o time -n 8 -n 8" ],
        ]
    },
    # --------------------------------------------------------------------------
    "mcnet": {
        dd_t.bdd: [
            # TODO: For now, we just set the time-limit to 2 days for our own
            #       experiments. Yet, this is too high for some and too low for
            #       others. We need to update these timings for another time!

            # Boolean Networks: AEON
            [ [ 2, 0,00], mcnet__args("aeon/[v5]__[r14]__[CORTICAL-AREA-DEVELOPMENT]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v5]__[r14]__[CORTICAL-AREA-DEVELOPMENT]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v5]__[r15]__[G2A]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v5]__[r15]__[G2A]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v6]__[r11]__[ZEBRA-MIR9-22-07-2011]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v6]__[r11]__[ZEBRA-MIR9-22-07-2011]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v9]__[r12]__[G2B]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v9]__[r12]__[G2B]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v9]__[r19]__[BUDDING-YEAST-ORLANDO-2008]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v9]__[r19]__[BUDDING-YEAST-ORLANDO-2008]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v9]__[r19]__[CELL-CYCLE-TRANSCRIPTION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v9]__[r19]__[CELL-CYCLE-TRANSCRIPTION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r14]__[AP-1-ELSE-0-WT]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r14]__[AP-1-ELSE-0-WT]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r18]__[ROOT-STEM-CELL-NICHE]__[biomodels].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r18]__[ROOT-STEM-CELL-NICHE]__[biomodels].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r27]__[FISSION-YEAST-DAVIDICH-2008]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r27]__[FISSION-YEAST-DAVIDICH-2008]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r34]__[BOOLEAN-CELL-CYCLE]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r34]__[BOOLEAN-CELL-CYCLE]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r35]__[MAMMALIAN-CELL-CYCLE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v10]__[r35]__[MAMMALIAN-CELL-CYCLE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v11]__[r11]__[TOLL-PATHWAY-12-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v11]__[r11]__[TOLL-PATHWAY-12-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v11]__[r11]__[TOLL-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v11]__[r11]__[TOLL-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v12]__[r30]__[METABOLIC-INTERACTIONS-IN-THE-GUT-MICROBIOME]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v12]__[r30]__[METABOLIC-INTERACTIONS-IN-THE-GUT-MICROBIOME]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v13]__[r18]__[REGULATION-OF-THE-LARABINOSE-OPERON]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v13]__[r18]__[REGULATION-OF-THE-LARABINOSE-OPERON]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v13]__[r22]__[LAC-OPERON]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v13]__[r22]__[LAC-OPERON]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v14]__[r66]__[ARABIDOPSIS-THALIANA-CELL-CYCLE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v14]__[r66]__[ARABIDOPSIS-THALIANA-CELL-CYCLE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v15]__[r38]__[CARDIAC-DEVELOPMENT]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v15]__[r38]__[CARDIAC-DEVELOPMENT]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v15]__[r38]__[PREDICTING-VARIABLES-IN-CARDIAC-GENE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v15]__[r38]__[PREDICTING-VARIABLES-IN-CARDIAC-GENE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v15]__[r66]__[FANCONI-ANEMIA-AND-CHECKPOINT-RECOVERY]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v15]__[r66]__[FANCONI-ANEMIA-AND-CHECKPOINT-RECOVERY]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r22]__[NEUROTRANSMITTER-SIGNALING-PATHWAY]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r22]__[NEUROTRANSMITTER-SIGNALING-PATHWAY]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r41]__[SKBR3-BREAST-CELL-LINE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r41]__[SKBR3-BREAST-CELL-LINE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r46]__[BT474-BREAST-CELL-LINE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r46]__[BT474-BREAST-CELL-LINE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r46]__[HCC1954-BREAST-CELL-LINE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r46]__[HCC1954-BREAST-CELL-LINE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r58]__[MAPK-RED3-19-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v16]__[r58]__[MAPK-RED3-19-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v17]__[r29]__[DROSOPHILA-BODY-SEGMENTATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v17]__[r29]__[DROSOPHILA-BODY-SEGMENTATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v17]__[r78]__[MAPK-RED1-19-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v17]__[r78]__[MAPK-RED1-19-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r18]__[VEGF-PATHWAY-12-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r18]__[VEGF-PATHWAY-12-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r18]__[VEGF-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r18]__[VEGF-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r43]__[TLGL-SURVIVAL-NETWORK]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r43]__[TLGL-SURVIVAL-NETWORK]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r58]__[BUDDING-YEAST-IRONS-2009]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r58]__[BUDDING-YEAST-IRONS-2009]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r59]__[BUDDING-YEAST-CELL-CYCLE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r59]__[BUDDING-YEAST-CELL-CYCLE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r60]__[MAPK-RED2-12-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r60]__[MAPK-RED2-12-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r78]__[CD4-T-CELL-DIFFERENTIATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r78]__[CD4-T-CELL-DIFFERENTIATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r83]__[T-CD4-LYMPHOCYTE-TRANSCRIPTION]__[biomodels].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v18]__[r83]__[T-CD4-LYMPHOCYTE-TRANSCRIPTION]__[biomodels].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r21]__[JNK-PATHWAY]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r21]__[JNK-PATHWAY]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r32]__[OXIDATIVE-STRESS-PATHWAY]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r32]__[OXIDATIVE-STRESS-PATHWAY]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r73]__[INFLAMMATORY-RESPONSES]__[biomodels].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r73]__[INFLAMMATORY-RESPONSES]__[biomodels].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r79]__[HUMAN-GONADAL-SEX-DETERMINATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v19]__[r79]__[HUMAN-GONADAL-SEX-DETERMINATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v20]__[r48]__[ERB-B2]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v20]__[r48]__[ERB-B2]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v20]__[r51]__[MAMMALIAN-CELL-CYCLE-1607]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v20]__[r51]__[MAMMALIAN-CELL-CYCLE-1607]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v20]__[r88]__[SUPP-MAT-MOD-NET]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v20]__[r88]__[SUPP-MAT-MOD-NET]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v21]__[r24]__[TGFB-pathway]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v21]__[r24]__[TGFB-pathway]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v22]__[r39]__[B-CELL-DIFFERENTIATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v22]__[r39]__[B-CELL-DIFFERENTIATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r24]__[FGF-PATHWAY-12-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r24]__[FGF-PATHWAY-12-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r24]__[FGF-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r24]__[FGF-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r34]__[T-CELL-DIFFERENTIATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r34]__[T-CELL-DIFFERENTIATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r43]__[AURORA-KINASE-A-IN-NEUROBLASTOMA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v23]__[r43]__[AURORA-KINASE-A-IN-NEUROBLASTOMA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r28]__[PROCESSING-OF-SPZ-NETWORK-FROM-DROSOPHILA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r28]__[PROCESSING-OF-SPZ-NETWORK-FROM-DROSOPHILA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r28]__[SPZ-PROCESSING-12-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r28]__[SPZ-PROCESSING-12-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r32]__[HH-PATHWAY-11-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r32]__[HH-PATHWAY-11-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r32]__[HH-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r32]__[HH-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r48]__[TOL-REGULATORY-NETWORK]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v24]__[r48]__[TOL-REGULATORY-NETWORK]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r67]__[S-PHASE-ENTRY-SIGNALLING-PATHWAY]__[biomodels].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r67]__[S-PHASE-ENTRY-SIGNALLING-PATHWAY]__[biomodels].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r70]__[BT474-BREAST-CELL-LINE-LONGTERM]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r70]__[BT474-BREAST-CELL-LINE-LONGTERM]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r70]__[HCC1954-BREAST-CELL-LINE-LONGTERM]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r70]__[HCC1954-BREAST-CELL-LINE-LONGTERM]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r81]__[SKBR3-BREAST-CELL-LINE-LONGTERM]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v25]__[r81]__[SKBR3-BREAST-CELL-LINE-LONGTERM]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r28]__[WG-PATHWAY-11-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r28]__[WG-PATHWAY-11-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r29]__[WG-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r29]__[WG-PATHWAY-OF-DROSOPHILA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r58]__[TRICHOSTRONGYLUS-RETORTAEFORMIS]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r58]__[TRICHOSTRONGYLUS-RETORTAEFORMIS]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r79]__[HSPC-MSC-0]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r79]__[HSPC-MSC-0]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r81]__[PROINFLAMMATORY-TUMOR-MICROENVIRONMENT]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v26]__[r81]__[PROINFLAMMATORY-TUMOR-MICROENVIRONMENT]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v28]__[r123]__[FA-BRCA-PATHWAY]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v28]__[r123]__[FA-BRCA-PATHWAY]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v28]__[r45]__[CALZONE-CELL-FATE]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v28]__[r45]__[CALZONE-CELL-FATE]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v28]__[r45]__[DEATH-RECEPTOR-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v28]__[r45]__[DEATH-RECEPTOR-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v29]__[r128]__[BLOOD-STEM-CELL-NETWORK]__[biomodels].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v29]__[r128]__[BLOOD-STEM-CELL-NETWORK]__[biomodels].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v31]__[r36]__[APOPTOSIS]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v31]__[r36]__[APOPTOSIS]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v31]__[r50]__[SEPARATION-INITIATION-NETWORK]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v31]__[r50]__[SEPARATION-INITIATION-NETWORK]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v32]__[r156]__[TUMOR-CELL-INVASION-AND-MIGRATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v32]__[r156]__[TUMOR-CELL-INVASION-AND-MIGRATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v33]__[r52]__[CELL-FATE-MULTISCALE]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v33]__[r52]__[CELL-FATE-MULTISCALE]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v33]__[r79]__[BORDETELLA-BRONCHISEPTICA]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v33]__[r79]__[BORDETELLA-BRONCHISEPTICA]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v33]__[r94]__[LYMPHOID-AND-MYELOID-CELL-SPECIFICATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v33]__[r94]__[LYMPHOID-AND-MYELOID-CELL-SPECIFICATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v34]__[r40]__[RTC-AND-TRANSCRIPTION]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v34]__[r40]__[RTC-AND-TRANSCRIPTION]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v34]__[r43]__[CHOLESTEROL-REGULATORY-PATHWAY]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v34]__[r43]__[CHOLESTEROL-REGULATORY-PATHWAY]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v38]__[r38]__[E-PROTEIN]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v38]__[r38]__[E-PROTEIN]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v38]__[r96]__[CD4-T-CELL-DIFFERENTIATION-6678]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v38]__[r96]__[CD4-T-CELL-DIFFERENTIATION-6678]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v40]__[r53]__[T-CELL-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v40]__[r53]__[T-CELL-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v40]__[r57]__[TCR-SIG-40]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v40]__[r57]__[TCR-SIG-40]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v41]__[r73]__[APOPTOSIS-NETWORK]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v41]__[r73]__[APOPTOSIS-NETWORK]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v42]__[r51]__[TREATMENT-OF-PROSTATE-CANCER]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v42]__[r51]__[TREATMENT-OF-PROSTATE-CANCER]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v44]__[r78]__[GUARD-CELL-ABSCISIC-ACID-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v44]__[r78]__[GUARD-CELL-ABSCISIC-ACID-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v47]__[r49]__[ORF3A]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v47]__[r49]__[ORF3A]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v48]__[r52]__[IFN-LAMBDA]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v48]__[r52]__[IFN-LAMBDA]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v49]__[r167]__[STOMATAL-OPENING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v49]__[r167]__[STOMATAL-OPENING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v50]__[r97]__[DIFFERENTIATION-OF-T-LYMPHOCYTES]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v50]__[r97]__[DIFFERENTIATION-OF-T-LYMPHOCYTES]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v51]__[r96]__[SENESCENCE-ASSOCIATED-SECRETORY-PHENOTYPE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v51]__[r96]__[SENESCENCE-ASSOCIATED-SECRETORY-PHENOTYPE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v52]__[r92]__[ORF10-CUL2-PATHWAY]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v52]__[r92]__[ORF10-CUL2-PATHWAY]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v53]__[r104]__[MAPK-CANCER-CELL-FATE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v53]__[r104]__[MAPK-CANCER-CELL-FATE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v53]__[r104]__[MAPK-LARGE-19-06-2013]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v53]__[r104]__[MAPK-LARGE-19-06-2013]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v53]__[r135]__[B-BRONCHISEPTICA-AND-T-RETORTAEFORMIS]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v53]__[r135]__[B-BRONCHISEPTICA-AND-T-RETORTAEFORMIS]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v60]__[r193]__[T-LGL]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v60]__[r193]__[T-LGL]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v60]__[r195]__[TLGL-SURVIVAL-NETWORK-2011]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v60]__[r195]__[TLGL-SURVIVAL-NETWORK-2011]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v60]__[r62]__[NSP4-NSP6]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v60]__[r62]__[NSP4-NSP6]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v61]__[r193]__[TLGL-SURVIVAL-NETWORK-2008]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v61]__[r193]__[TLGL-SURVIVAL-NETWORK-2008]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v62]__[r108]__[PC12-CELL-DIFFERENTIATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v62]__[r108]__[PC12-CELL-DIFFERENTIATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v66]__[r128]__[IMMUNE-CHECKPOINT-NETWORK]__[DOI-10.3390-cancers12123600].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v66]__[r128]__[IMMUNE-CHECKPOINT-NETWORK]__[DOI-10.3390-cancers12123600].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v66]__[r139]__[SIGNALING-PATHWAY-FOR-BUTHANOL-PRODUCTION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v66]__[r139]__[SIGNALING-PATHWAY-FOR-BUTHANOL-PRODUCTION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v67]__[r131]__[BORTEZOMIB-RESPONSES-IN-U266]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v67]__[r131]__[BORTEZOMIB-RESPONSES-IN-U266]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v68]__[r103]__[HGF-SIGNALING-IN-KERATINOCYTES]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v68]__[r103]__[HGF-SIGNALING-IN-KERATINOCYTES]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v70]__[r153]__[COLITISASSOCIATE-COLON-CANCER]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v70]__[r153]__[COLITISASSOCIATE-COLON-CANCER]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v73]__[r114]__[YEAST-APOPTOSIS]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v73]__[r114]__[YEAST-APOPTOSIS]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v73]__[r97]__[GLUCOSE-REPRESSION-SIGNALING-2009]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v73]__[r97]__[GLUCOSE-REPRESSION-SIGNALING-2009]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v81]__[r160]__[LIMPHOPOIESIS-REGULATORY-NETWORK]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v81]__[r160]__[LIMPHOPOIESIS-REGULATORY-NETWORK]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v85]__[r130]__[RENIN-ANGIOTENSIN]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v85]__[r130]__[RENIN-ANGIOTENSIN]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v86]__[r149]__[IL6-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v86]__[r149]__[IL6-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v91]__[r104]__[PAMP-SIGNALING]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v91]__[r104]__[PAMP-SIGNALING]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v94]__[r132]__[PYRIMIDINE-DEPRIVATION]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v94]__[r132]__[PYRIMIDINE-DEPRIVATION]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v101]__[r158]__[T-CELL-RECEPTOR-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v101]__[r158]__[T-CELL-RECEPTOR-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v102]__[r157]__[INTERFERON-1]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v102]__[r157]__[INTERFERON-1]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v104]__[r166]__[ETC]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v104]__[r166]__[ETC]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v104]__[r226]__[EGFR-ERBB-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v104]__[r226]__[EGFR-ERBB-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v110]__[r212]__[RODRIGUEZ-JORGE-TCR-SIGNALLING-17-07-2018]__[ginsim].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v110]__[r212]__[RODRIGUEZ-JORGE-TCR-SIGNALLING-17-07-2018]__[ginsim].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v118]__[r218]__[IL1-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v118]__[r218]__[IL1-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v120]__[r195]__[COAGULATION-PATHWAY]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v120]__[r195]__[COAGULATION-PATHWAY]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v131]__[r302]__[INFLUENZA-VIRUS-REPLICATION-CYCLE]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v131]__[r302]__[INFLUENZA-VIRUS-REPLICATION-CYCLE]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v139]__[r557]__[SIGNAL-TRANSDUCTION-IN-FIBROBLASTS]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v165]__[r275]__[VIRUS-REPLICATION-CYCLE]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v165]__[r275]__[VIRUS-REPLICATION-CYCLE]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v168]__[r243]__[HMOX1-PATHWAY]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v168]__[r243]__[HMOX1-PATHWAY]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v188]__[r351]__[CD4-T-CELL-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v188]__[r351]__[CD4-T-CELL-SIGNALING]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v193]__[r328]__[KYNURENINE-PATHWAY]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v193]__[r328]__[KYNURENINE-PATHWAY]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v205]__[r269]__[ER-STRESS]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v205]__[r269]__[ER-STRESS]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v247]__[r1100]__[ERBB-14-RECEPTOR-SIGNALING]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v260]__[r258]__[NSP9-PROTEIN]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v260]__[r258]__[NSP9-PROTEIN]__[covid-uni-lu].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v321]__[r533]__[SIGNALING-IN-MACROPHAGE-ACTIVATION]__[cellcollective].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v321]__[r533]__[SIGNALING-IN-MACROPHAGE-ACTIVATION]__[cellcollective].aeon", True)],
            [ [ 2, 0,00], mcnet__args("aeon/[v351]__[r737]__[NSP14]__[covid-uni-lu].aeon", False)],
            [ [ 2, 0,00], mcnet__args("aeon/[v351]__[r737]__[NSP14]__[covid-uni-lu].aeon", True)],

            # Boolean Networks: BNET
            [ [ 2, 0,00], mcnet__args("bnet/arellano_rootstem.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/arellano_rootstem.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/calzone_cellfate.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/calzone_cellfate.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/dahlhaus_neuroplastoma.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/dahlhaus_neuroplastoma.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/davidich_yeast.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/davidich_yeast.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/dinwoodie_life.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/dinwoodie_life.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/dinwoodie_stomatal.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/dinwoodie_stomatal.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/faure_cellcycle.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/faure_cellcycle.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/grieco_mapk.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/grieco_mapk.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/irons_yeast.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/irons_yeast.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/jaoude_thdiff.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/jaoude_thdiff.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/klamt_tcr.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/klamt_tcr.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/krumsiek_myeloid.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/krumsiek_myeloid.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/multivalued.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/multivalued.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/n12c5.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/n12c5.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/n3s1c1a.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/n3s1c1a.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/n3s1c1b.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/n3s1c1b.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/n5s3.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/n5s3.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/n6s1c2.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/n6s1c2.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/n7s3.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/n7s3.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/raf.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/raf.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/randomnet_n15k3.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/randomnet_n15k3.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/randomnet_n7k3.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/randomnet_n7k3.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/remy_tumorigenesis.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/remy_tumorigenesis.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/remy_tumorigenesis_myversion.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/remy_tumorigenesis_myversion.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/saadatpour_guardcell.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/saadatpour_guardcell.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/selvaggio_emt.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/selvaggio_emt.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/tournier_apoptosis.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/tournier_apoptosis.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/xiao_wnt5a.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/xiao_wnt5a.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/zhang_tlgl.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/zhang_tlgl.bnet", True)],
            [ [ 2, 0,00], mcnet__args("bnet/zhang_tlgl_v2.bnet", False)],
            [ [ 2, 0,00], mcnet__args("bnet/zhang_tlgl_v2.bnet", True)],

            # MCC (Petri Net) 2023: Anderson
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_04.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_04.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_05.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_06.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_07.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_08.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_09.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_10.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_11.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/Anderson/anderson_12.pnml", False)],


            # MCC (Petri Net) 2023: EisenbergMcGuire
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_03.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_03.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_04.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_04.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_05.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_06.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_07.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_08.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_09.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2023/EisenbergMcGuire/eisenberg-mcguire_10.pnml", False)],

            # MCC (Petri Net) 2022: AutonomousCar
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar01_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar01_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar01_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar01_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar02_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar02_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar02_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar02_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar03_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar03_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar03_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar03_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar04_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar04_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar04_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar04_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar05_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar05_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar05_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar05_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar06_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar06_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar06_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar06_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar07_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar07_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar07_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar07_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar08_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar08_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar08_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar08_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar09_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar09_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar10_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/AutonomousCar/autocar10_b.pnml", False)],

            # MCC (Petri Net) 2022: RERS2020
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem101.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem102.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem103.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem104.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem105.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem106.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem107.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem108.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/RERS2020/problem109.pnml", False)],

            # MCC (Petri Net) 2022: StigmergyCommit
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm02-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm02-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm02-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm03-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm03-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm03-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm04-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm04-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm05-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm05-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm04-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm05-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm06-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm06-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm06-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm07-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm07-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm07-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm08-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm08-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm09-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm09-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm10-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm10-b.pnml", False)], # 435.5 Mib !
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm11-a.pnml", False)], # 486.3 MiB !
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyCommit/stigcomm11-b.pnml", False)], # 1.4 GiB   !

            # MCC (Petri Net) 2022: StigmergyElection
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec02-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec02-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec02-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec02-b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec03-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec03-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec04-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec04-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec04-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec05-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec05-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec05-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec06-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec06-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec06-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec07-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec07-a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec07-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec08-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec08-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec09-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec09-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec10-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec10-b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec11-a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2022/StigmergyElection/stigelec11-b.pnml", False)], # 432.0 MiB !

            # MCC (Petri Net) 2021: GPUForwardProgress
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_04_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_04_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_04_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_04_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_08_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_08_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_08_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_08_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_12_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_12_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_12_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_12_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_16_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_16_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_16_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_16_b.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_20_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_20_a.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_20_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_24_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_24_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_28_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_28_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_32_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_32_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_36_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_36_b.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_40_a.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/GPUForwardProgress/gpufp_40_b.pnml", False)],

            # MCC (Petri Net) 2021: HealthRecord
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_01.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_01.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_02.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_02.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_03.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_03.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_04.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_04.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_05.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_05.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_06.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_06.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_07.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_07.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_08.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_08.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_09.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_09.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_10.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_10.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_11.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_11.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_12.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_12.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_13.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_13.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_14.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_14.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_15.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_15.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_16.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_16.pnml", True)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_17.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/HealthRecord/hrec_17.pnml", True)],

            # MCC (Petri Net) 2021: ServersAndClients
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-100-020.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-100-040.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-100-080.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-100-160.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-100-320.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-200-040.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-200-080.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-200-160.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-200-320.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-400-080.pnml", False)],
            [ [ 2, 0,00], mcnet__args("mcc/2021/ServersAndClients/ServersAndClients-400-160.pnml", False)]#,

            # MCC (Petri Net) 2020: ShieldIIPs
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_001_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_001_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_001_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_001_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_002_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_002_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_002_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_002_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_003_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_003_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_003_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_003_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_004_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_004_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_004_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_004_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_005_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_005_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_005_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_010_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_010_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_010_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_020_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_020_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_020_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_030_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_030_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_030_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_040_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_040_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_040_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_040_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_050_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_050_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_050_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_100_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_100_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPs/shield_s_iip_100_b.pnml", False)],

            # MCC (Petri Net) 2020: ShieldIIPt
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_001_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_001_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_001_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_002_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_002_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_002_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_002_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_003_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_003_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_004_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_004_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_005_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_005_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_005_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_010_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_010_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_020_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_020_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_030_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_030_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_040_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_040_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_050_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_050_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_100_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldIIPt/shield_t_iip_100_b.pnml", False)],

            # MCC (Petri Net) 2020: ShieldPPPs
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_001_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_001_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_001_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_002_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_002_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_002_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_002_b.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_003_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_003_a.pnml", True)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_003_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_004_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_004_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_005_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_005_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_010_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_010_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_020_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_020_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_030_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_030_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_040_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_040_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_050_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_050_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_100_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPs/shield_s_ppp_100_b.pnml", False)],

            # MCC (Petri Net) 2020: ShieldPPPt
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_001_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_001_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_002_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_002_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_003_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_003_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_004_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_004_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_005_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_005_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_010_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_010_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_020_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_020_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_030_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_030_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_040_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_040_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_050_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_050_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_100_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/ShieldPPPt/shield_t_ppp_100_b.pnml", False)],

            # MCC (Petri Net) 2020: SmartHome
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_01.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_02.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_05.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_03.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_04.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_06.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_07.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_08.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_09.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_10.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_11.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_12.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_13.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_14.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_15.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_16.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_17.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_18.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2020/SmartHome/smhome_19.pnml", False)],

            # MCC (Petri Net) 2019: NoC3x3
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_2_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_7_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_5_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_4_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_1_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_8_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_3_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_6_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_3_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_6_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_4_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_8_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_1_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_5_a.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_2_b.pnml", False)],
          # [ [ 2, 0,00], mcnet__args("mcc/2019/NoC3x3/noc3x3_7_a.pnml", False)]
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
            [ [ 0, 0,10], qbf__args("B/2x4_13_bwnib") ],
            [ [ 0, 0,10], qbf__args("B/2x5_17_bwnib") ],
            [ [ 0, 0,10], qbf__args("B/2x6_15_bwnib") ],
            [ [ 1, 0,00], qbf__args("B/3x4_19_bwnib") ],
            [ [ 0, 0,10], qbf__args("B/3x5_11_bwnib") ],
            [ [ 0, 0,10], qbf__args("B/3x6_9_bwnib") ],
            # BSP/ (Breakthrough 2nd Player)
            [ [ 0, 0,10], qbf__args("BSP/2x4_8_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/3x4_12_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/2x5_10_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/3x5_12_bwnib") ],
            [ [ 0, 1,00], qbf__args("BSP/2x6_14_bwnib") ],
            [ [ 0, 0,10], qbf__args("BSP/3x6_10_bwnib") ],
            # C4/ (Connect 4)
            [ [ 0, 0,10], qbf__args("C4/2x2_3_connect2_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/3x3_3_connect2_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/3x3_9_connect3_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/4x4_3_connect2_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/4x4_9_connect3_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/4x4_15_connect4_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/5x5_3_connect2_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/5x5_9_connect3_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/5x5_11_connect4_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/6x6_3_connect2_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/6x6_9_connect3_bwnib") ],
            [ [ 0, 0,10], qbf__args("C4/6x6_11_connect4_bwnib") ],
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
            [ [ 0, 0,30], qbf__args("D/4x6_12_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x2_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x3_8_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x4_10_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/5x5_13_bwnib") ],
            [ [ 0, 0,30], qbf__args("D/5x6_11_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/6x2_6_bwnib") ],
            [ [ 0, 0,10], qbf__args("D/6x3_10_bwnib") ],
            [ [ 0, 0,30], qbf__args("D/6x4_12_bwnib") ],
            [ [ 0, 0,30], qbf__args("D/6x5_11_bwnib") ],
            [ [ 0,12,00], qbf__args("D/6x6_11_bwnib") ],
            # EP/ (Evader-Pursuer)
            [ [ 0, 0,30], qbf__args("EP/4x4_3_e-4-1_p-2-3_bwnib") ],
            [ [ 0,12,00], qbf__args("EP/4x4_21_e-4-1_p-1-2_bwnib") ],
            [ [ 0, 0,30], qbf__args("EP/8x8_7_e-8-1_p-3-4_bwnib") ],
            [ [ 0,12,00], qbf__args("EP/8x8_11_e-8-1_p-2-3_bwnib") ],
            # EP-dual/ (Evader-Pursuer from the other's perspective)
            [ [ 0, 0,10], qbf__args("EP-dual/4x4_2_e-4-1_p-1-2_bwnib") ],
            [ [ 0, 0,10], qbf__args("EP-dual/4x4_10_e-4-1_p-2-3_bwnib") ],
            [ [ 0, 0,10], qbf__args("EP-dual/8x8_6_e-8-1_p-2-3_bwnib") ],
            [ [ 0, 4,00], qbf__args("EP-dual/8x8_10_e-8-1_p-3-4_bwnib") ],
            # hex/ (Hex)
            [ [ 0, 0,10], qbf__args("hex/browne_5x5_07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/browne_5x5_09_bwnib") ],
            [ [ 0,12,00], qbf__args("hex/hein_02_5x5-11_bwnib") ],
            [ [ 1, 0,00], qbf__args("hex/hein_02_5x5-13_bwnib") ],
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
            [ [ 0,12,00], qbf__args("hex/hein_10_5x5-11_bwnib") ],
            [ [ 1, 0,00], qbf__args("hex/hein_10_5x5-13_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_11_5x5-09_bwnib") ],
            [ [ 2, 0,00], qbf__args("hex/hein_11_5x5-11_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_12_4x4-05_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_12_4x4-07_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_13_5x5-07_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_13_5x5-09_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_14_5x5-07_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_14_5x5-09_bwnib") ],
            [ [ 0,12,00], qbf__args("hex/hein_15_5x5-13_bwnib") ],
            [ [ 0,12,00], qbf__args("hex/hein_15_5x5-15_bwnib") ],
            [ [ 0,12,00], qbf__args("hex/hein_16_5x5-11_bwnib") ],
            [ [ 4, 0,00], qbf__args("hex/hein_16_5x5-13_bwnib") ],
            [ [ 0, 0,10], qbf__args("hex/hein_19_5x5-09_bwnib") ],
            [ [ 0, 0,30], qbf__args("hex/hein_19_5x5-11_bwnib") ],
            # httt/ (High-dimensional Tic-Tac-Toe)
            [ [ 0, 0,10], qbf__args("httt/3x3_3_domino_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_5_el_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_elly_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_fatty_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_knobby_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_tic_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/3x3_9_tippy_bwnib") ],
            [ [ 0, 0,30], qbf__args("httt/4x4_3_domino_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_5_el_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_5_tic_bwnib") ],
            [ [ 0, 0,10], qbf__args("httt/4x4_7_elly_bwnib") ],
            [ [ 0, 0,30], qbf__args("httt/4x4_9_tippy_bwnib") ],
            [ [ 0,12,00], qbf__args("httt/4x4_11_knobby_bwnib") ],
            [ [ 0,12,00], qbf__args("httt/4x4_13_skinny_bwnib") ],
            [ [ 0,12,00], qbf__args("httt/4x4_15_fatty_bwnib") ],
        ]
    },
    # --------------------------------------------------------------------------
    "queens": {
        dd_t.bdd: [
            [ [ 0, 0,10], "-n 4"  ],
            [ [ 0, 0,10], "-n 5"  ],
            [ [ 0, 0,10], "-n 6"  ],
            [ [ 0, 0,10], "-n 7"  ],
            [ [ 0, 0,10], "-n 8"  ],
            [ [ 0, 0,10], "-n 9"  ],
            [ [ 0, 0,10], "-n 10" ],
            [ [ 0, 0,30], "-n 11" ],
            [ [ 0, 1, 0], "-n 12" ],
            [ [ 0, 2, 0], "-n 13" ],
            [ [ 0, 3, 0], "-n 14" ],
            [ [ 0, 3, 0], "-n 15" ],
            [ [ 0,20, 0], "-n 16" ],
            [ [ 3, 0, 0], "-n 17" ],
            [ [15, 0, 0], "-n 18" ],
         ],
        dd_t.zdd: [
            [ [ 0, 0,10], "-n 4"  ],
            [ [ 0, 0,10], "-n 5"  ],
            [ [ 0, 0,10], "-n 6"  ],
            [ [ 0, 0,10], "-n 7"  ],
            [ [ 0, 0,10], "-n 8"  ],
            [ [ 0, 0,10], "-n 9"  ],
            [ [ 0, 0,10], "-n 10" ],
            [ [ 0, 0,30], "-n 11" ],
            [ [ 0, 1, 0], "-n 12" ],
            [ [ 0, 2, 0], "-n 13" ],
            [ [ 0, 3, 0], "-n 14" ],
            [ [ 0, 3, 0], "-n 15" ],
            [ [ 0,20, 0], "-n 16" ],
            [ [ 3, 0, 0], "-n 17" ],
            [ [15, 0, 0], "-n 18" ],
        ]
    },
    # --------------------------------------------------------------------------
    "tic-tac-toe": {
        dd_t.bdd: [
            [ [ 0, 0,10], "-n 13" ],
            [ [ 0, 0,10], "-n 14" ],
            [ [ 0, 0,10], "-n 15" ],
            [ [ 0, 0,10], "-n 16" ],
            [ [ 0, 0,10], "-n 17" ],
            [ [ 0, 0,10], "-n 18" ],
            [ [ 0, 0,10], "-n 19" ],
            [ [ 0, 0,10], "-n 20" ],
            [ [ 0, 0,30], "-n 21" ],
            [ [ 0, 2, 0], "-n 22" ],
            [ [ 0, 4, 0], "-n 23" ],
            [ [ 0,12, 0], "-n 24" ],
            [ [ 1,12, 0], "-n 25" ],
            [ [ 4, 0, 0], "-n 26" ],
            [ [ 6, 0, 0], "-n 27" ],
            [ [ 8, 0, 0], "-n 28" ],
            [ [15,00, 0], "-n 29" ],
        ],
        dd_t.zdd: [
            [ [ 0, 0,10], "-n 13" ],
            [ [ 0, 0,10], "-n 14" ],
            [ [ 0, 0,10], "-n 15" ],
            [ [ 0, 0,10], "-n 16" ],
            [ [ 0, 0,10], "-n 17" ],
            [ [ 0, 0,10], "-n 18" ],
            [ [ 0, 0,10], "-n 19" ],
            [ [ 0, 0,10], "-n 20" ],
            [ [ 0, 0,30], "-n 21" ],
            [ [ 0, 2, 0], "-n 22" ],
            [ [ 0, 4, 0], "-n 23" ],
            [ [ 0,12, 0], "-n 24" ],
            [ [ 1,12, 0], "-n 25" ],
            [ [ 4, 0, 0], "-n 26" ],
            [ [ 6, 0, 0], "-n 27" ],
            [ [ 9, 0, 0], "-n 28" ],
            [ [15,00, 0], "-n 29" ],
        ]
    },
}

# Copy BDD timings to BCDD timiings
for b in BENCHMARKS.keys():
    if dd_t.bdd in BENCHMARKS[b].keys():
        BENCHMARKS[b][dd_t.bcdd] = BENCHMARKS[b][dd_t.bdd]

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

partitions = {
             # Mem, CPU
    "q20":    [128, "ivybridge",      15],
    "q20fat": [128, "ivybridge",      15],
    "q24":    [256, "haswell",        28],
    "q28":    [256, "broadwell",      15],
    "q36":    [384, "skylake",        15],
    "q40":    [384, "cascadelake",    15],
    "q48":    [384, "cascadelake",    15],
    "q64":    [512, "icelake-server", 15],
}

partition = "q48"
partition_choice = input("Grendel Node (default: 'q48'): ")
if partition_choice:
    if not partition_choice in partitions.keys():
        print(f"Partition '{partition_choice}' is unknown")
        exit(-1)
    partition = partition_choice

try:
    time_factor = float(input("Time Limit Factor (default: 1.0): "))
except:
    time_factor = 1.0

def time_limit_scale(t):
    hours_to_mins = 60
    days_to_mins = 24 * hours_to_mins

    max_time = partitions[partition][2] * days_to_mins

    total_minutes = t[0] * days_to_mins + t[1] * hours_to_mins + t[2]
    scaled_minutes = min(time_factor * total_minutes, max_time)
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
                    time_key = time_limit_str(time_limit_scale(instance[0]))
                    grouped_instances.setdefault(time_key, []).append([p, benchmark, dd, instance[1]])

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

    memory = partitions[partition][0]
    memory = int(memory - memory/10) * 1024

    slurm_content = f'''#!/bin/bash
{sbatch_str(f"{slurm_job_prefix}__{slurm_job_suffix}", time, True)}
#SBATCH --array=1-{len(benchmarks)}

awk '{awk_array_idx} {{ system("touch {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("echo -e \\"\\n=========  Started `date`  ==========\\n\\" | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

awk '{awk_array_idx} {{ system("{SLURM_ORIGIN}/build/src/"$2 {awk_args} " -M {memory} -T /scratch/{SLURM_JOB_ID} 2>&1 | tee -a {SLURM_ORIGIN}/"$1) }}' {SLURM_ORIGIN}/grendel/{awk_name}

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

# 'cbindgen' for Rust-to-C FFI
cargo install --force cbindgen
export PATH=~/.cargo/bin/:$PATH

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
