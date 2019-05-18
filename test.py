import voice

v=voice.Voice(512)
result=v.filter(5)
print('result='+str(result))
print('v', dir(v))
result=v.doubleIt(5)
print('result='+str(result))
