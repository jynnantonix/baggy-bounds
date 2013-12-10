#!/usr/bin/python
import os
import subprocess
import time
import sys

BENCHMARK_FILES = ["seating","qsort"]

FAILCOLOR = '\033[91m'
OKCOLOR = '\033[92m'
ENDCOLOR = '\033[0m'

def main():
    benchmark_mode = ("--benchmark" in sys.argv)

    directory = os.path.dirname(os.path.realpath(__file__))
    srcdir = os.path.join(directory, "tests")
    inputdir = os.path.join(directory, "inputs")
    outputdir = os.path.join(directory, "outputs")

    if benchmark_mode:
        progs = BENCHMARK_FILES
    else:
        progs = [os.path.basename(filename).split('.')[0] for filename in os.listdir(srcdir) if filename.endswith('.cpp')]
        progs = [p for p in progs if p not in BENCHMARK_FILES]
    progs.sort()

    for prog in progs:
        inputs = [os.path.join(inputdir, filename) for filename in os.listdir(inputdir) if filename.startswith(prog+'.in')]

        proc = subprocess.Popen(["make", "tests/%s.bg.out" % prog], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if proc.wait() != 0:
            print "Test %s: %sFAILED%s to compile tests/%s.bg.out" % (prog, FAILCOLOR, ENDCOLOR, prog)
            continue
        if benchmark_mode:
            proc = subprocess.Popen(["make", "tests/%s.out" % prog], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if proc.wait() != 0:
                print "Test %s: %sFAILED%s to compile tests/%s.out" % (prog, FAILCOLOR, ENDCOLOR, prog)
                continue
            proc = subprocess.Popen(["make", "tests/%s.clangout" % prog], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if proc.wait() != 0:
                print "Test %s: %sFAILED%s to compile tests/%s.clangout" % (prog, FAILCOLOR, ENDCOLOR, prog)
                continue

        if not inputs:
            res = test_input("tests/%s.bg.out" % prog, None, "outputs/%s.out" % prog, prog)
            if benchmark_mode:
                res1 = test_input("tests/%s.out" % prog, None, "outputs/%s.out" % prog, prog)
                res2 = test_input("tests/%s.clangout" % prog, None, "outputs/%s.out" % prog, prog)
                print_benchmark(prog, "", res, res1, res2)
            else:
                print_test(prog, "", res)
        else:
            for inp in sorted(inputs):
                suffix = os.path.basename(inp)[len(prog+'.in'):]
                res = test_input("tests/%s.bg.out" % prog, inp, "outputs/%s.out%s" % (prog, suffix), prog)
                if benchmark_mode:
                    res1 = test_input("tests/%s.out" % prog, inp, "outputs/%s.out%s" % (prog, suffix), prog)
                    res2 = test_input("tests/%s.clangout" % prog, inp, "outputs/%s.out%s" % (prog, suffix), prog)
                    print_benchmark(prog, suffix, res, res1, res2)
                else:
                    print_test(prog, suffix, res)

def print_test(prog, suffix, res):
    suff = suffix and (" (" + prog + ".in" + suffix + ")")
    if isinstance(res, str):
        print "Test %s%s: %sFAILED%s (%s)" % (prog, suff, FAILCOLOR, ENDCOLOR, res)
    else:
        print "Test %s%s: %sPASSED%s" % (prog, suff, OKCOLOR, ENDCOLOR)

def print_benchmark(prog, suffix, res, res1, res2):
    print "Test %s%s:" % (prog, suffix and (" (" + prog + ".in" + suffix + ")"))
    if isinstance(res2, str):
        print "    clang++:       %sFAILED%s (%s)" % (FAILCOLOR, ENDCOLOR, res2)
    else:
        print "    clang++:       %sPASSED%s (%.6f sec)" % (OKCOLOR, ENDCOLOR, res2)
    if isinstance(res1, str):
        print "    without baggy: %sFAILED%s (%s)" % (FAILCOLOR, ENDCOLOR, res1)
    else:
        print "    without baggy: %sPASSED%s (%.6f sec)" % (OKCOLOR, ENDCOLOR, res1)
    if isinstance(res, str):
        print "    with baggy:    %sFAILED%s (%s)" % (FAILCOLOR, ENDCOLOR, res)
    else:
        print "    with baggy:    %sPASSED%s (%.6f sec)" % (OKCOLOR, ENDCOLOR, res)

env = dict(os.environ)
env["BAGGY"] = "YES"
def test_input(executablefile, inpfile, outfile, basename):
    if inpfile:
        inpf = open(inpfile, "r")
    else:
        inpf = subprocess.PIPE
    start = time.time()
    proc = subprocess.Popen([executablefile, "sample_argument"], stdin=inpf, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    (out, err) = proc.communicate()
    ret_code = proc.wait()
    end = time.time()

    if inpfile:
        inpf.close()

    out_lines = [line+"\n" for line in out.split('\n')[:-1] if "debug" not in line]

    outf = open(outfile, "r")
    outfile_lines = list(outf)
    desired_out_lines = outfile_lines[:-1]
    desired_ret_code = int(outfile_lines[-1].strip())
    outf.close()

    if len(out_lines) != len(desired_out_lines):
        return "output mismatch - wrong number of lines"
    if any(x != y and y != "?\n" for x, y in zip(out_lines, desired_out_lines)):
        return "output mismatch"
    if ret_code != desired_ret_code:
        return "return code %d, expected %d" % (ret_code, desired_ret_code)
    return end - start

if __name__ == '__main__':
    main()
