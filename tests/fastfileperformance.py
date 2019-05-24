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

timedifference = time.time() - timenow
timedifference = datetime.timedelta( seconds=timedifference )
print( 'Python   timedifference', timedifference, flush=True )

timenow = time.time()
iterable = fastfilepackage.FastFile( testfile )
for item in iterable:
    if 'word' in item:
        pass

timedifference = time.time() - timenow
timedifference = datetime.timedelta( seconds=timedifference )
print( 'FastFile timedifference', timedifference, flush=True )
