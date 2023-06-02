.PHONY: all build clean

build: | clean
	@python3 grendel_gen.py
	@chmod a+x *.sh

clean:
	@rm -f *.awk
	@rm -f *.sh
