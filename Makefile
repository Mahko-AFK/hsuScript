.PHONY: test

test:
	echo "===== Running language tests ====="
	./tools/runexec.sh
	echo "===== Running execution tests ====="
	./tools/runexec.sh
	