MODULE = walksat


.PHONY: run build clean

run: build
	time mpiexec -n 8 python main.py formula.cnf
	minisat formula.cnf

build: clean
	python -m pip install -e .

clean:
	python -m pip uninstall -y $(MODULE)
	rm -rf \
		build \
		$(MODULE).egg-info \
		$(MODULE)/*.so $(MODULE)/*.c
