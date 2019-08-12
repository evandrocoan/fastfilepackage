import fastfilepackage
iterable = fastfilepackage.FastFile( './sample.txt' )
results = []
for item in iterable:
    print( 'a) %s' % iterable() )
    print( 'b) %s' % iterable() )
    print( 'c) %s' % iterable() )
    iterable.resetlines()
    print( 'd) %s' % iterable() )

