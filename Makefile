clean-pyc:
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	find . -name '*~'    -exec rm -f {} +

clean-cpp:
	find . -name '*.c'   -exec rm -f {} +
	find . -name '*.o'   -exec rm -f {} +
	find . -name '*.so'  -exec rm -f {} +
	find . -name '*.cpp' -exec rm -f {} +

clean-general:
	find . -name '.DS_Store'   -exec rm -f {} +
	find . -name '.idea'   -exec rm -f -r {} +

clean-build:
	rm -f -r build/
	rm -f -r dist/
	rm -f -r *.egg-info


.PHONY: clean
clean: clean-build clean-general clean-cpp clean-pyc

.PHONY: install
install: clean
	pip install -e .
