.PHONY: clean build test

MAKE_FLAGS=-j $$(nproc)

# ============================================================================ #
#  VARIABLE DEFAULTS
# ============================================================================ #

# Variant (adiar, buddy, sylvan)
V:=buddy

# Memory
M:=128

# Instance size variable (custom default for every target below)
N:=0

# ============================================================================ #
#  BUILD TARGETS
# ============================================================================ #
build:
  # Primary build
	@echo "\nBuild"
	@mkdir -p build/ && cd build/ && cmake ..

  # Installation of sylvan
	@echo "\n\nInstall Sylvan"
	@cd build/sylvan && make DESTDIR=./ && make install DESTDIR=./

build-pigeonhole_principle: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_pigeonhole_principle

build-queens: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_queens

build-queens_sat: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_queens_sat

build-tic_tac_toe: | build
	@cd build/ && make ${MAKE_FLAGS} ${V}_tic_tac_toe


clean:
	@rm -rf build/


# ============================================================================ #
#  RUN TARGETS
# ============================================================================ #
run-pigeonhole_principle: N := 10
run-pigeonhole_principle:
	@$(subst VARIANT,$(V),./build/src/VARIANT_pigeonhole_principle) $(N) $(M)

run-queens: N := 8
run-queens:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens) $(N) $(M)

run-queens_sat: N := 8
run-queens_sat:
	@$(subst VARIANT,$(V),./build/src/VARIANT_queens_sat) $(N) $(M)

run-tic_tac_toe: N := 20
run-tic_tac_toe:
	@$(subst VARIANT,$(V),./build/src/VARIANT_tic_tac_toe) $(N) $(M)

# ============================================================================ #
#  ALL-IN-ONE TARGETS
# ============================================================================ #
pigeonhole_principle: build-pigeonhole_principle run-pigeonhole_principle
queens: build-queens run-queens
queens_sat: build-queens_sat run-queens_sat
tic_tac_toe: build-tic_tac_toe run-tic_tac_toe

