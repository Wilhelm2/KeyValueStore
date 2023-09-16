import subprocess

def getTime():
    with open("results",'r') as file:
        lines = [line.rstrip() for line in file]
        userTime = lines[1].split("\t")[1].split("m")
        sysTime = lines[2].split("\t")[1].split("m")
        minutes = int(userTime[0]) + int(sysTime[0])
        seconds = float(userTime[1][:-1].replace(",",".")) + float(sysTime[1][:-1].replace(",","."))
        return str(minutes) + "minutes " +  str(seconds) +" seconds "
        
def doTest(hashFunction, nbAdds):
    subprocess.check_call(["bash","-c","(time ./main database "+str(nbAdds) + " 10 FIRST_FIT \"r+\" " + str(hashFunction) + ") 2> results"])
    print "Hashing " + str(hashFunction) + " " + getTime()
    subprocess.check_call(["bash","-c","rm data*"])	
    subprocess.check_call(["bash","-c","rm results"])	
	
 
# Tests the different hash methods 
# Inserts 50000 keys to the database whose size is between 0 and 10, then deletes 25000 of them 
# print "First test"
subprocess.check_call(["bash","-c","make random"])	
doTest(0,500)
# doTest(2,40000)
# doTest(3,40000)

# Test the different hash methods 
# Inserts 4000 keys whose size is between 0 and 10, then deletes 2000 of them 
# print "Second test"
# subprocess.check_call(["bash","-c","make zero"])	
# doTest(0,4000)
# doTest(2,4000)
# doTest(3,4000)
