# Apply (*.binary.bdd*)

Minimal (hand-encoded) binary BDDs on the format it is serialized by *lib-bdd*.

- *bot.bdd* : &perp;
- *top.bdd* : &top;
- *x0.bdd* : x<sub>0</sub>
- *x1.bdd* : x<sub>1</sub>
- *x2.bdd* : x<sub>2</sub>
- *x255.bdd* : x<sub>255</sub> (checks parsing is unsigned)
- *x0_and_x1.bdd* : x<sub>0</sub> &wedge; x<sub>1</sub>
- *x0_or_x1.bdd* : x<sub>0</sub> &vee; x<sub>1</sub>
- *x0_or_x1.bdd* : x<sub>0</sub> &vee; x<sub>1</sub>
- *x0_xor_x1.bdd* : x<sub>0</sub> &oplus; x<sub>1</sub>
- *x0_xnor_x1.bdd* : x<sub>0</sub> &equiv; x<sub>1</sub>
- *topological.bdd* : (to test code to deal with BDD nodes not being level-by-level)

Furthermore, there are also minimal (hand-encoded) binary ZDDs in the same format.

- *empty.zdd* : Ø
- *000.zdd* : { Ø }
- *100.zdd* : { {0} }
- *010.zdd* : { {1} }
- *001.zdd* : { {2} }
- *000_100.zdd* : { Ø, {0} }
- *100_010.zdd* : { {0}, {1} }
- *110.zdd* : { {0,1} }

## More Inputs

- Pastva & Henzinger (2023): https://zenodo.org/records/7958052
