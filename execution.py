import subprocess

def getTime():
    with open("results",'r') as file:
        lines = [line.rstrip() for line in file]
        userTime = lines[1].split("\t")[1].split("m")
        sysTime = lines[2].split("\t")[1].split("m")
        minutes = int(userTime[0]) + int(sysTime[0])
        seconds = float(userTime[1][:-1].replace(",",".")) + float(sysTime[1][:-1].replace(",","."))
        return str(minutes) + "minutes " +  str(seconds) +" seconds "
        
    
	

print "Premier test"

# Tests the different hash methods 
# Adds 50000 keys to the database whose size is between 0 and 10, then deletes 25000 of them 
subprocess.check_call(["bash","-c","make random"])	
subprocess.check_call(["bash","-c","(time ./main database 40000 10 FIRST_FIT \"r+\" 0) 2> results"])
print "Default hashing " + getTime()
subprocess.check_call(["bash","-c","rm data*"])	


subprocess.check_call(["bash","-c","make random"])	
subprocess.check_call(["bash","-c","(time ./main database 40000 10 FIRST_FIT \"r+\" 2) 2> results"])
print "Second hashing " + getTime()
subprocess.check_call(["bash","-c","rm data*"])	

subprocess.check_call(["bash","-c","make random"])	
subprocess.check_call(["bash","-c","(time ./main database 40000 10 FIRST_FIT \"r+\" 3) 2> results"])
print "Third hashing " + getTime()
subprocess.check_call(["bash","-c","rm data*"])	
subprocess.check_call(["bash","-c","rm results"])	

