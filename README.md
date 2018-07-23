# negainoido icfpc 2018!!

## One time setup

### Require
* Bazel
* (gcloud)

## Build and Test

### Build
* `bazel build //...`


### Run
* `bazel run //solver:itigo -- --src_filename=- --tgt_filename=FA001_tgt.mdl > nbt.nbt`

#### solvers
* itigo
* duskin_ai
* large_udon_ai
* simple_solve
* square_delete
* oscar_ai
* udon_ai
* crimea_ai
* skelton
* vertial_ai
* cube_eraser
* kevin_ai
* bestcat
