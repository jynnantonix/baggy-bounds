import sys
from subprocess import *

def passes():
    binfile = sys.argv[1]
    outfile = sys.argv[2]

    proc = Popen([binfile], stdout=PIPE, stderr=PIPE)
    out, err = proc.communicate()
    ret_code = proc.wait()

    out_lines = [line+"\n" for line in out.split('\n')[:-1] if "debug" not in line]

    f = open(outfile)
    outfile_lines = list(f)
    desired_out_lines = outfile_lines[:-1]
    desired_ret_code = int(outfile_lines[-1].strip())

    if len(out_lines) != len(desired_out_lines):
        return False
    if any(x != y and y != "?\n" for x, y in zip(out_lines, desired_out_lines)):
        return False
    if ret_code != desired_ret_code:
        return False
    return True

if passes():
    exit(0)
else:
    exit(1)
