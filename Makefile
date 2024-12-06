MODULE = walksat


.PHONY: run build clean

run: build
	time mpiexec -n 12 python main.py

build: clean
	python -m pip install -e .

clean:
	python -m pip uninstall -y $(MODULE)
	rm -rf \
		build \
		$(MODULE).egg-info \
		$(MODULE)/*.so $(MODULE)/*.c
