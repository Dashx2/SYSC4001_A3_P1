if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101306172_101265573_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101306172_101265573_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101306172_101265573_EP_RR.cpp

# If no argument, just compile and exit
if [ $# -eq 0 ]; then
    echo "Compiled EP, RR, EP_RR. To run a test: ./build.sh test6.txt"
    exit 0
fi

# --- 2. Run tests and rename execution files ---

INPUT_ARG="$1"

# Allow either "test6" or "test6.txt"
if [[ "$INPUT_ARG" == *.txt ]]; then
    INPUT_FILE="$INPUT_ARG"
else
    INPUT_FILE="${INPUT_ARG}.txt"
fi

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: input file '$INPUT_FILE' not found."
    exit 1
fi

# Extract the number (6 from test6, etc.) for naming
TEST_NUM=$(echo "$INPUT_FILE" | grep -o '[0-9]\+')

if [ -z "$TEST_NUM" ]; then
    echo "Error: could not extract test number from '$INPUT_FILE'"
    exit 1
fi

OUT_PREFIX="execution_test${TEST_NUM}"

# Run each scheduler and rename execution.txt accordingly
for SCHED in EP RR EP_RR; do
    echo "Running $SCHED on $INPUT_FILE..."
    ./bin/interrupts_"$SCHED" "$INPUT_FILE"
    mv -f execution.txt "${OUT_PREFIX}_${SCHED}.txt"
done

echo "Done. Created:"
echo "  ${OUT_PREFIX}_EP.txt"
echo "  ${OUT_PREFIX}_RR.txt"
echo "  ${OUT_PREFIX}_EP_RR.txt"