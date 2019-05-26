import io
import time
import datetime
import fastfilepackage

# usually a file with 100MB
testfile = './myfile.log'

timenow = time.time()
iterable = io.open( testfile, 'r' )
for item in iterable:
    if None:
        var = item

python_time = time.time() - timenow
timedifference = datetime.timedelta( seconds=python_time )
print( 'Python   timedifference', timedifference, flush=True )

timenow = time.time()
iterable = fastfilepackage.FastFile( testfile )
for item in iterable:
    if None:
        item = var

fastfile_time = time.time() - timenow
timedifference = datetime.timedelta( seconds=fastfile_time )
print( 'FastFile timedifference', timedifference, flush=True )
print( 'fastfile_time %.2f%%, python_time %.2f%% = %.2f%%' % (
        fastfile_time/python_time, python_time/fastfile_time,
        abs( 1 - python_time/fastfile_time ) ), flush=True )
