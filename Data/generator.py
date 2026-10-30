import random
from datetime import datetime, timedelta

START = datetime(1980, 1, 1, 0, 0, 0)
END = datetime(2026, 1, 1, 0, 0, 0)   # exclusive
TOTAL_SECONDS = int((END - START).total_seconds())
N = 100000
SEED = 50
OUTPUT_FILE = f"Data/timestamps_100k.txt"
TIME_FORMAT = "%Y-%m-%d %H:%M:%S"

random.seed(SEED)

# N distinct offsets, already in random order (never sorted)
offsets = random.sample(range(TOTAL_SECONDS), N)

with open(OUTPUT_FILE, "w") as f:
    for offset in offsets:
        f.write((START + timedelta(seconds=offset)).strftime(TIME_FORMAT) + "\n")

print(f"{OUTPUT_FILE} generated with {N} timestamps")

# Verification: format, range, uniqueness and non-sorted order
with open(OUTPUT_FILE) as f:
    lines = [line.strip() for line in f]

dates = [datetime.strptime(line, TIME_FORMAT) for line in lines]  # fails on invalid format/date
assert len(lines) == N, "wrong number of lines"
assert len(set(lines)) == N, "duplicate timestamps found"
assert all(START <= d < END for d in dates), "date out of range"
assert dates != sorted(dates), "file is sorted (it should not be)"

print("Verification OK: valid format, within range, no duplicates, not sorted")