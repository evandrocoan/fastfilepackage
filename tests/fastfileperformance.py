import time
import datetime
import fastfilepackage

# usually a file with 100MB
testfile = './myfile.log'

timenow = time.time()
with open( testfile, 'r', errors='replace' ) as myfile:
    for item in myfile:
        if 'word' in item:
            pass

python_time = time.time() - timenow
timedifference = datetime.timedelta( seconds=python_time )
print( 'Python   timedifference', timedifference, flush=True )

timenow = time.time()
iterable = fastfilepackage.FastFile( testfile )
for item in iterable:
    if 'word' in item:
        pass

fastfile_time = time.time() - timenow
timedifference = datetime.timedelta( seconds=fastfile_time )
print( 'FastFile timedifference', timedifference, flush=True )
print( 'fastfile_time %.2f%%, python_time %.2f%%' % (
        fastfile_time/python_time, python_time/fastfile_time ), flush=True )
