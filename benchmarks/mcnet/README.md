# McNet

Minimal (hand-crafted) examples of inputs.

### Boolean Networks (*.aeon*)

- *pyboolnet.a.aeon* : First example in the
  [PyBoolNet Documentation](https://pyboolnet.readthedocs.io/en/master/quickstart.html)
  translated by hand into AEON.
- *pyboolnet.b.aeon* : Second example in the
  [PyBoolNet Documentation](https://pyboolnet.readthedocs.io/en/master/quickstart.html)
  translated by hand into AEON.
- *split.aeon* : A simple net with a split. Depending on whether it is
  interpreted synchronously or asynchronously is visible in the number of
  reachable states.
- *wiki.aeon* : The example of a Boolean Network on
  [Wikipedia](https://en.wikipedia.org/wiki/Boolean_network#/media/File:Hou710_BooleanNetwork.svg)

### Boolean Networks (*.bnet*)

- *pyboolnet.a.bnet* : First example in the
  [PyBoolNet Documentation](https://pyboolnet.readthedocs.io/en/master/quickstart.html)
- *pyboolnet.b.bnet* : Second example in the
  [PyBoolNet Documentation](https://pyboolnet.readthedocs.io/en/master/quickstart.html)
- *split.bnet* : A simple net with a split. Depending on whether it is
  interpreted synchronously or asynchronously is visible in the number of
  reachable states.
- *wiki.bnet* : The example of a Boolean Network on
  [Wikipedia](https://en.wikipedia.org/wiki/Boolean_network#/media/File:Hou710_BooleanNetwork.svg)

### Boolean Networks (*.sbml* qual)

- *complex.sbml* : A Boolean network with more multi-term encoding of transitions.
- *pyboolnet.a.bnet* : First example in the
  [PyBoolNet Documentation](https://pyboolnet.readthedocs.io/en/master/quickstart.html)
  translated by hand into SBML.
- *pyboolnet.b.bnet* : Second example in the
  [PyBoolNet Documentation](https://pyboolnet.readthedocs.io/en/master/quickstart.html)
  translated by hand into SBML.
- *merge.sbml* : The *merge.pnml* Petri net translated into an asynchronous Boolean
  Network.
- *split.sbml* : The *split.pnml* Petri net translated into an asynchronous Boolean
  Network. Using synchronous semantics should differ in the number of states.
- *wiki.sbml* : The example of a Boolean Network on
  [Wikipedia](https://en.wikipedia.org/wiki/Boolean_network#/media/File:Hou710_BooleanNetwork.svg)

### Petri Nets (*.pnml*)

- *0-cycle.pnml* : A single place with no transitions
- *1-cycle.pnml* : A simple cycle of length 1
- *2-cycle.pnml* : A simple cycle of length 2
- *3-cycle.pnml* : A simple cycle of length 3
- *4-cycle.pnml* : A simple cycle of length 4
- *merge.pnml* : Simple merge of two tokens into one.
- *scc.a.pnml* : The SCC example from Fig. 1 in the TACAS 23 paper "A Truly Symbolic Linear-Time
  Algorithm for SCC Decomposition" by Larsen et al. .
- *self-loop.pnml* : Net with a self-loop on an initial place
  (needed to find all reachable states).
- *sink-transition.pnml* : Net with a sink transition, i.e. an outgoing
  transitios without an output.
- *split.pnml* : Net with a non-deterministic split to two places.
- *unreachable.1.pnml* : Net with two trivially unreachable place
  (neither initial and by propagation of not being a target).
- *unreachable.2.pnml* : Net with a non-trivially unreachable set of places
  (would require graph traversal to identify)
- *unreachable.3.pnml* : Net with trivially unreachable set of places
  (nothing is marked initial)

## More Inputs

### Boolean Networks (*aeon*)

- AEON models repository :
  https://github.com/sybila/biodivine-lib-param-bn/tree/master/sbml_models

### Boolean Networks (*.bnet*)

- PyBoolNet model repository :
  https://github.com/hklarner/pyboolnet/tree/master/pyboolnet/repository

### Boolean Networks (*sbml* qual)

- EMBL-EBI’s BioModels model repository :
  https://www.ebi.ac.uk/biomodels/

- SBML models repository :
  https://github.com/sybila/biodivine-lib-param-bn/tree/master/sbml_models

### Petri Nets (*.pnml*)

- Model Checking Competition (2024): https://mcc.lip6.fr/2024/models.php

  *1-Safe* Petri Nets:
  - [Anderson](https://mcc.lip6.fr/2024/archives/Anderson-pnml.tar.gz)
  - [EisenbergMcGuire](https://mcc.lip6.fr/2024/archives/EisenbergMcGuire-pnml.tar.gz)
  - [AutonomousCar](https://mcc.lip6.fr/2024/archives/AutonomousCar-pnml.tar.gz)
  - [RERS2020](https://mcc.lip6.fr/2024/archives/RERS2020-pnml.tar.gz)
  - [StigmergyCommit](https://mcc.lip6.fr/2024/archives/StigmergyCommit-pnml.tar.gz)
  - [StigmergyElection](https://mcc.lip6.fr/2024/archives/StigmergyElection-pnml.tar.gz)
  - [GPUForwardProcess](https://mcc.lip6.fr/2024/archives/GPUForwardProgress-pnml.tar.gz)
  - [HealthRecord](https://mcc.lip6.fr/2024/archives/HealthRecord-pnml.tar.gz)
  - [ServerAndClients](https://mcc.lip6.fr/2024/archives/ServersAndClients-pnml.tar.gz)
  - [ShieldIIPs](https://mcc.lip6.fr/2024/archives/ShieldIIPs-pnml.tar.gz)
  - [ShieldIIPt](https://mcc.lip6.fr/2024/archives/ShieldIIPt-pnml.tar.gz)
  - [ShieldPPPs](https://mcc.lip6.fr/2024/archives/ShieldPPPs-pnml.tar.gz)
  - [ShieldPPPt](https://mcc.lip6.fr/2024/archives/ShieldPPPt-pnml.tar.gz)
  - [SmartHome](https://mcc.lip6.fr/2024/archives/SmartHome-pnml.tar.gz)
  - [NoC3x3](https://mcc.lip6.fr/2024/archives/NoC3x3-pnml.tar.gz)
  - [ASLink](https://mcc.lip6.fr/2024/archives/ASLink-pnml.tar.gz)
  - [BusinessProcesses](https://mcc.lip6.fr/2024/archives/BusinessProcesses-pnml.tar.gz)
  - [DLCflexbar](https://mcc.lip6.fr/2024/archives/DLCflexbar-pnml.tar.gz)
  - [DiscoveryGPU](https://mcc.lip6.fr/2024/archives/DiscoveryGPU-pnml.tar.gz)
  - [EGFr](https://mcc.lip6.fr/2024/archives/EGFr-pnml.tar.gz)
  - [MAPKbis](https://mcc.lip6.fr/2024/archives/MAPKbis-pnml.tar.gz)
  - [NQueens](https://mcc.lip6.fr/2024/archives/NQueens-pnml.tar.gz)
  - [CloudReconfiguration](https://mcc.lip6.fr/2024/archives/CloudReconfiguration-pnml.tar.gz)
  - [DLCround](https://mcc.lip6.fr/2024/archives/DLCround-pnml.tar.gz)
  - [FlexibleBarrier](https://mcc.lip6.fr/2024/archives/FlexibleBarrier-pnml.tar.gz)
  - [AutoFlight](https://mcc.lip6.fr/2024/archives/AutoFlight-pnml.tar.gz)
  - [CloudDeployment](https://mcc.lip6.fr/2024/archives/CloudDeployment-pnml.tar.gz)
  - [DES](https://mcc.lip6.fr/2024/archives/DES-pnml.tar.gz)
  - [DLCshifumi](https://mcc.lip6.fr/2024/archives/DLCshifumi-pnml.tar.gz)
  - [IBM319](https://mcc.lip6.fr/2024/archives/IBM319-pnml.tar.gz)
  - [IBM703](https://mcc.lip6.fr/2024/archives/IBM703-pnml.tar.gz)
  - [Parking](https://mcc.lip6.fr/2024/archives/Parking-pnml.tar.gz)
  - [Raft](https://mcc.lip6.fr/2024/archives/Raft-pnml.tar.gz)
  - [ARMCacheCoherence](https://mcc.lip6.fr/2024/archives/ARMCacheCoherence-pnml.tar.gz)
  - [EnergyBus](https://mcc.lip6.fr/2024/archives/EnergyBus-pnml.tar.gz)
  - [MultiwaySync](https://mcc.lip6.fr/2024/archives/MultiwaySync-pnml.tar.gz)
  - [ParamProductionCell](https://mcc.lip6.fr/2024/archives/ParamProductionCell-pnml.tar.gz)
  - [ProductionCell](https://mcc.lip6.fr/2024/archives/ProductionCell-pnml.tar.gz)
  - [UtahNoC](https://mcc.lip6.fr/2024/archives/UtahNoC-pnml.tar.gz)
  - [Dekker](https://mcc.lip6.fr/2024/archives/Dekker-pnml.tar.gz)
  - [ResAllocation](https://mcc.lip6.fr/2024/archives/ResAllocation-pnml.tar.gz)
  - [Vasy2003](https://mcc.lip6.fr/2024/archives/Vasy2003-pnml.tar.gz)
  - [Echo](https://mcc.lip6.fr/2024/archives/Echo-pnml.tar.gz)
  - [Eratosthenes](https://mcc.lip6.fr/2024/archives/Eratosthenes-pnml.tar.gz)
  - [Railroad](https://mcc.lip6.fr/2024/archives/Railroad-pnml.tar.gz)
  - [Ring](https://mcc.lip6.fr/2024/archives/Ring-pnml.tar.gz)
  - [RwMutex](https://mcc.lip6.fr/2024/archives/RwMutex-pnml.tar.gz)
  - [SimpleLoadBal](https://mcc.lip6.fr/2024/archives/SimpleLoadBal-pnml.tar.gz)

  *1-safe* and *Coloured* Petri Nets:
  - [Sudoku](https://mcc.lip6.fr/2024/archives/Sudoku-pnml.tar.gz)
  - [BART](https://mcc.lip6.fr/2024/archives/BART-pnml.tar.gz)
  - [Referendum](https://mcc.lip6.fr/2024/archives/Referendum-pnml.tar.gz)
  - [AirplaneLD](https://mcc.lip6.fr/2024/archives/AirplaneLD-pnml.tar.gz)
  - [SafeBus](https://mcc.lip6.fr/2024/archives/SafeBus-pnml.tar.gz)
  - [DotAndBoxes](https://mcc.lip6.fr/2024/archives/DotAndBoxes-pnml.tar.gz)
  - [DrinkVendingMachine](https://mcc.lip6.fr/2024/archives/DrinkVendingMachine-pnml.tar.gz)
  - [LamportFastMutEx](https://mcc.lip6.fr/2024/archives/LamportFastMutEx-pnml.tar.gz)
  - [NeoElection](https://mcc.lip6.fr/2024/archives/NeoElection-pnml.tar.gz)
  - [Peterson](https://mcc.lip6.fr/2024/archives/Peterson-pnml.tar.gz)
  - [Philosophers](https://mcc.lip6.fr/2024/archives/Philosophers-pnml.tar.gz)
  - [SharedMemory](https://mcc.lip6.fr/2024/archives/SharedMemory-pnml.tar.gz)
  - [TokenRing](https://mcc.lip6.fr/2024/archives/TokenRing-pnml.tar.gz)
