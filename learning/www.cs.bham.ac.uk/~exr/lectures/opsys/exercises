#!/bin/bash

#
# Operating Systems 2015
# University of Birmingham
#
# copyright: Michael Denzel <m.denzel@cs.bham.ac.uk>
#

### CONFIG ###
target=/tmp/
assignment_num=3
##############

# --- helper function ---
function opsys_testcase(){
	#allow port 80 for netcat and port 22 for telnet
	t="80 /bin/nc\n22 /usr/bin/telnet\n"
	client=./firewallSetup
	d=/proc/firewallExtension
	executable=firewallExtension
	rules=/tmp/newFirewallRules
	
	#cleanup
	rmmod $executable 2>/dev/null >/dev/null
	
	#load kernel driver
	echo "loading driver"
	insmod ./$executable.ko
	if [ $? -ne 0 ]
	then
		echo "ERROR: insmod failed"
	    return -1
	fi

	#test script
	{
		#create rules
		rm -f $rules
		echo -en $t > $rules
		if [ $? -ne 0 ]
		then
			echo "ERROR: cannot create file $rules"
			return -1
		fi
		chmod +x $rules
		if [ $? -ne 0 ]
		then
			echo "ERROR: chmod returned $?"
			return -1
		fi

		#write rules
		$client W /tmp/newFirewallRules 2>&1 | grep "ERROR" > err
		tmp=`cat err`
		if [ $? -ne 0 ] || [ "$tmp" != "" ]
		then
			echo -e "ERROR: $client returned error"
			echo -e "(`cat err`)"
			return -1
		fi

		#list rules
		$client L 2>&1 | grep "ERROR" > err
		tmp=`cat err`
		if [ $? -ne 0 ] || [ "$tmp" != "" ]
		then
			echo -e "ERROR: $client returned error"
			echo -e "(`cat err`)"
			return -1
		fi
		dm=`dmesg | grep "Firewall rule:" | tail -n 2 | sed 's/\[[0-9.\ ]*\]\ Firewall rule:\ \(.*\)/\1/'`
		tmp=`cat $rules`
		if [ "$tmp" != "$dm" ]
		then
			echo -e "ERROR: read firewall rules do not match $rules"
			echo -e "dmesg:\n$dm"
			echo -e "file:\n$tmp"
			return -1
		fi
	}
	
	#unload module
	echo "unloading driver"
	rmmod $executable
	if [ $? -ne 0 ]
	then
		echo "ERROR: unloading kernel module failed"
		return -1
	fi

	#check unloading
	ret=`ls $d 2>/dev/null`
	if [ "$ret" != "" ]
	then
		echo "ERROR: $d still exists after unloading"
		return -1
	fi
	
	return 0
}

# --- MAIN ---

#check
if [ $# -ne 1 ]
then
	echo "Syntax: $(basename $0) <path to zip file>"
	echo "zip file structure:"
	echo -e "\t<studentID>.zip\n\t└── assignment3\n\t    ├── Makefile\n\t    └── ..."
	exit -1
fi

#init
studentID=`echo "$1" | sed 's/\([0-9]\{7\}\).*/\1/'`
filetype=`file -b "$1" | awk '{print $1;}'`
cwd=`pwd`
zipfile=$(basename $1)

#check studentID
if [[ "$studentID" =~ ^[0-9]{7}$ ]]
then
	#check filetype
	if [ "$filetype" == "Zip" ]
	then
		#reset
		rm -rf $target/$studentID
		mkdir $target/$studentID

		#copy zip
		cp $1 $target/$studentID

		#unzip
		echo "unzipping"
		cd $target/$studentID
		unzip $zipfile >/dev/null 2>/dev/null
		if [ $? -ne 0 ]
		then
			echo "ERROR: unzip failed"
			exit -1
		fi

		#goto folder
		cd ./assignment$assignment_num
		if [ $? -ne 0 ]
		then
			echo "ERROR: zip file should include a folder assignment$assignment_num"
			exit -1
		fi

		#execute makefile
		echo "executing Makefile"
		make -f ./Makefile >/dev/null
		if [ $? -ne 0 ]
		then
			echo "ERROR: make failed"
			exit -1
		fi

		#basic test
		echo "basic test"
		export -f opsys_testcase
		su -c opsys_testcase
		if [ $? -eq 0 ]
		then
			echo "SUCCESS: unzip, make, basic test"
			echo "(please be aware that we will test more cases than the basic test)"
			exit 0
		else
			exit $?
		fi
	else
		echo "ERROR: Not a zip file"
		exit -1
	fi
else
	echo "ERROR: StudentID seems wrong"
	exit -1
fi

