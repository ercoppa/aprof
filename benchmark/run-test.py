import os
import sys
import os
import subprocess

class Benchmark:

    def __init__(self):
        # path
        self.path = "."
        # aprof binary path
        self.aprof = os.path.dirname(os.path.realpath(__file__)) + "/../valgrind/inst/bin/valgrind "
        # valgrin/aprof options
        self.aprof_opts = " --tool=aprof --log-dir=" + os.path.dirname(os.path.realpath(__file__)) + "/tests-logs/ "
        # how to build the benchmark
        self.build = []
        # how to run it
        self.run = []
        # how to check success or failure
        self.check = ['make clean']

    def execute_cmd(self, cmd, log = None):
        cmd = "cd " + self.path + " && time " + cmd
        if log is None:
            os.system(cmd)
        else:
            subprocess.call(cmd, shell=True, stdout=log, stderr=log)

    def execute(self):
        log = open("tests-logs/" + self.name + ".log", "w")
        print "Building benchmark: " + self.name 
        for cmd in self.build:
            self.execute_cmd(cmd, log)
        print "Running benchmark: " + self.name 
        for cmd in self.run:
            cmd = self.aprof + self.aprof_opts + cmd
            self.execute_cmd(cmd, log)
        print "Checking benchmark: " + self.name 
        for cmd in self.check:
            self.execute_cmd(cmd, log)
        log.close()

if __name__ == "__main__":
    
    os.system("rm -rf tests-logs && mkdir tests-logs")
    benchmarks = []

    dsp = Benchmark()
    dsp.name = "dsp"
    dsp.path = "dsp/"
    dsp.build = ["make"]
    dsp.run = ["bin/dsp < scripts/qaprof_benchmark1.txt"]
    benchmarks.append(dsp)

    quick = Benchmark()
    quick.name = "quicksort"
    quick.path = "quicksort/"
    quick.build = ["make"]
    quick.run = ["./quick_sort"]
    benchmarks.append(quick)

    buffread = Benchmark()
    buffread.name = "buffered-read"
    buffread.path = "buffered-read/"
    buffread.build = ["make"]
    buffread.run = ["./read"]
    buffread.aprof_opts += " --drms=yes "
    benchmarks.append(buffread)

    buff_read_mmap = Benchmark()
    buff_read_mmap.name = "buffered-read-mmap"
    buff_read_mmap.path = "buffered-read-mmap/"
    buff_read_mmap.build = ["make"]
    buff_read_mmap.run = ["./read-mmap"]
    buff_read_mmap.aprof_opts += " --drms=yes "
    benchmarks.append(buff_read_mmap)

    fwrite = Benchmark()
    fwrite.name = "fwrite"
    fwrite.path = "fwrite/"
    fwrite.build = ["make"]
    fwrite.run = ["./fwrite"]
    fwrite.aprof_opts += " --drms=yes "
    benchmarks.append(fwrite)

    prod_cons = Benchmark()
    prod_cons.name = "producer-consumer"
    prod_cons.path = "prod-cons-only-thread-input/"
    prod_cons.build = ["make"]
    prod_cons.run = ["./consumer-producer", "./consumer-producer"]
    prod_cons.aprof_opts += " --drms=yes --incremental-log=yes "
    benchmarks.append(prod_cons)

    for b in benchmarks:
        b.execute()
