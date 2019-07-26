# To avoid accidentally deleting files, default is to print smtg :)
noop:
	@echo "SpatialIndex: Use 'make install' or 'make clean'"

install:
	pip install -e .

clean: clean-build clean-general clean-cpp clean-pyc clean-tests

clean-pyc:
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	find . -name '*~'    -exec rm -f {} +
	rm -rf __pycache__

clean-cpp:
	find . -name '*.c'   -exec rm -f {} +
	find . -name '*.o'   -exec rm -f {} +
	find . -name '*.so'  -exec rm -f {} +

clean-general:
	find . -name '.DS_Store'   -exec rm -f {} +
	find . -name '.idea'   -exec rm -f -r {} +

clean-build:
	rm -f -r build/
	rm -f -r dist/
	rm -f -r *.egg-info

clean-tests:
	rm -rf .tox .eggs
