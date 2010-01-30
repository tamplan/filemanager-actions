#!/bin/sh
#
# A script to check that we have as much instance_dispose that instance_init
# in our debug traces.
#
# $1 = input file (debug log)

if [ "$1" = "" ]; then
	echo "Usage: $0 <logfile>" 1>&2
	exit 1
fi

tmp1=/tmp/$(basename $0).$$.1
tmp2=/tmp/$(basename $0).$$.2
tmp3=/tmp/$(basename $0).$$.3

\rm -f /tmp/$(basename $0).*

# get in tmp1 just the list of instance_init/instance_dispose with line numbers
# it is kept in run order (line number order)
grep -En 'instance_init|instance_dispose' ${1} | grep -v "base_window_instance_dispose: quitting main window" > ${tmp1}
#cat ${tmp1}
#exit

# get in tmp2 the same run-ordered lines, just with line numbers as a separated field
cat ${tmp1} | while read line; do
	numline=$(echo ${line} | cut -d: -f1)
	echo "${numline} $(echo ${line} | cut -d' ' -f3-)" >> ${tmp2}
done
#cat ${tmp2}
#exit

# get in tmp3 the same population of lines where fields are separated to prepare the sort
# fields in the line are yet in the same order as in tmp1 and tmp2 files
# 1: numline
# 2: function name
# 3: object address
#    (note that the same address may be reused several times in the program)
# 4: object type name
#    (note that instance_init of a base class use the base class name
#    while the instance_dispose use the type name of the derived class)
# 5: radical of function name (e.g. base_application)
# 6: an help for the sort
cat ${tmp2} | while read line; do
	numline=$(echo ${line} | awk '{ print $1 }')
	fname=$(echo ${line} | awk '{ print $2 }' | sed 's/:$//')
	address=$(echo ${line} | awk '{ print $3}' | sed -e 's/.*=//' -e 's/,$//')
	typename=$(echo ${line} | awk '{ print $4 }' | sed 's/[(),]*//g')
	[ "${typename}" = "" ] && echo "warning: no type in line ${line}" 1>&2
	prefix=$(echo ${fname} | sed 's/_instance.*$//')
	nature=1:init
	[ "$(echo ${fname} | grep init)" = "" ] && nature=2:disp
	#echo "address='${address}' fname='${fname}' object='${object}' typename='${typename}' numline='${numline}'" >> ${tmp3}
	#echo -e "${numline}\t ${fname}\t ${address}\t ${typename}\t ${prefix}\t ${rang}" >> ${tmp3}
	#echo "${numline} ${fname} ${address} ${typename} ${prefix} ${nature}" >> ${tmp3}
	echo "${numline} ${fname} ${address} ${typename} ${prefix} ${nature}" >> ${tmp3}
done

export LC_ALL=C
sortparms="-k3,3 -k5,5 -k1,1n"
#sortparms="-k3 -k5 -k6"
#cat ${tmp3} | sort ${sortparms}
#exit

line_init=""
cat ${tmp3} | sort ${sortparms} | while read line; do
	nature=$(echo ${line} | awk '{ print $6 }')
	if [ "${line_init}" = "" -a "${nature}" != "1:init" ]; then
		echo "warning: unwaited line: ${line}"
		continue
	fi
	if [ "${line_init}" = "" ]; then
		adr_init=$(echo ${line} | awk '{ print $3 }')
		app_init=$(echo ${line} | awk '{ print $5 }')
		line_init="${line}"
	else
		adr_dispose=$(echo ${line} | awk '{ print $3 }')
		app_dispose=$(echo ${line} | awk '{ print $5 }')
		line_dispose="${line}"
		if [ "${adr_dispose}" = "${adr_init}" ]; then
			numline_init=$(echo ${line_init} | awk '{ print $1 }')
			numline_dispose=$(echo ${line_dispose} | awk '{ print $1 }')
			type_init=$(echo ${line_init} | awk '{ print $4 }')
			echo "${type_init} ${adr_init} (${numline_init},${numline_dispose}): OK"
			line_init=""
		else
			echo "warning: not apparied line: ${line_init}"
			adr_init=${adr_dispose}
			app_init=${adr_dispose}
			line_init="${line_dispose}"
		fi
	fi
done
