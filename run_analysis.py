"""Do some analysis of FFT timing"""
import sys

import matplotlib.pyplot as plt

def get_monitor_file(filename):
    """Find the most recent log file"""
    with open(filename, "r") as file:
        return [line.strip() for line in file]


def extract_file_contents(file_contents):
    """Extract the file contents (i.e. the numbers) from the lines."""
    assert file_contents[-1].startswith("Completed")

    def make_num(num_str):
        try:
            return int(num_str)
        except ValueError:
            return None

    return [micros for micro_str in file_contents if (micros := make_num(micro_str))]


def main(filename):
    """Do stuff"""
    file_contents = get_monitor_file(filename)
    micros = extract_file_contents(file_contents)

    fig, ax = plt.subplots(1, 1, figsize=(16, 9))
    ax.hist(micros, bins=100)

    fig, ax = plt.subplots(1, 1, figsize=(16, 9))
    ax.set_ylabel("micros")
    ax.plot(micros)

    plt.show()


if __name__ == "__main__":
    main(sys.argv[1])
