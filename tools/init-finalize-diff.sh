#!/bin/sh
#
# A script to check that we have as many instance_dispose that instance_init
# in our debug traces.
#
# $1 = input file (debug log)
#
#real	6m1.314s
#user	0m27.317s
#sys	1m28.583s

function usage 
{
	echo "Usage: $0 <logfile> [display_ok=true|false]" 1>&2
}

if [ "$1" = "" ]; then
	usage
	exit 1
fi

if [ "${2}" = "" ]; then
	display_ok=1
elif [ "${2}" = "display_ok=false" ]; then
	display_ok=0
elif [ "${2}" != "display_ok=true" ]; then
	usage
	exit 1
fi

tmp1=/tmp/$(basename $0).$$.1
tmp2=/tmp/$(basename $0).$$.2
tmp3=/tmp/$(basename $0).$$.3

\rm -f /tmp/$(basename $0).*
echo ""

function getftmp
{
	echo /tmp/$(basename $0).$$.${1}
}

function str_init
{
	typeset ftmp=$(getftmp ${1})
	echo "" > ${ftmp}
}

function str_grep
{
	typeset ftmp=$(getftmp ${1})
	grep -w $2 ${ftmp}
}

function str_add
{
	typeset ftmp=$(getftmp ${1})
	echo ${2} >>${ftmp}
}

function count_init
{
	typeset ftmp=$(getftmp ${1})
	echo "0">${ftmp}
}

function count_inc
{
	typeset ftmp=$(getftmp ${1})
	typeset value=$(cat ${ftmp})
	let value+=1
	echo ${value}>${ftmp}
}

function count_display
{
	typeset ftmp=$(getftmp ${1})
	cat ${ftmp}
}

function is_reused
{
	typeset fused=/tmp/$(basename $0).$$.reused
	typeset triplet="${1}"
	typeset -i count=0

	typeset used_line="$(grep -w ${triplet} ${fused} 2>/dev/null)"
	if [ "${used_line}" = "" ]; then
		count=1
		echo "${triplet} ${count}" >> ${fused}
	else
		count=$(echo ${used_line} | awk '{ print $2 }')
		let count+=1
		grep -v ${triplet} ${fused} > ${fused}.tmp
		echo "${triplet} ${count}" >> ${fused}.tmp
		mv ${fused}.tmp ${fused}
	fi

	echo ${count}
}

# get in tmp1 just the list of instance_init/instance_dispose with line numbers
# it is kept in run order (line number order)
echo -n "Extracting and formatting relevant lines from ${1}"
count100=0
count500=0
total=0
grep -En 'instance_init|instance_dispose' ${1} | grep -vE 'quitting main window|parent=|children=|tree=|deleted=' | while read line; do
	let total+=1
	let count100+=1
	if [ ${count100} -ge 100 ]; then
		echo -n "."
		count100=0
	fi
	let count500+=1
	if [ ${count500} -ge 500 ]; then
		echo -n " ${total}"
		count500=0
	fi
	numline=$(echo ${line} | cut -d: -f1)
	#echo "${numline} $(echo ${line} | cut -d' ' -f3-)" >> ${tmp1}
	#numline=$(echo ${line} | awk '{ print $1 }')
	fname=$(echo ${line} | awk '{ print $3 }' | sed 's/:$//')
	address=$(echo ${line} | awk '{ print $4 }' | sed -e 's/.*=//' -e 's/,$//')
	typename=$(echo ${line} | awk '{ print $5 }' | sed 's/[(),]*//g')
	[ "${typename}" = "" ] && echo "warning: no type in line ${line}" 1>&2
	prefix=$(echo ${fname} | sed 's/_instance.*$//')
	nature=1:init
	[ "$(echo ${fname} | grep init)" = "" ] && nature=2:disp
	reused=$(is_reused ${address}-${prefix}-${nature})
	#echo "address='${address}' fname='${fname}' object='${object}' typename='${typename}' numline='${numline}'" >> ${tmp3}
	#echo -e "${numline}\t ${fname}\t ${address}\t ${typename}\t ${prefix}\t ${rang}" >> ${tmp3}
	#echo "${numline} ${fname} ${address} ${typename} ${prefix} ${nature}" >> ${tmp3}
	echo "${numline} ${fname} ${address} ${typename} ${prefix} ${nature} ${reused}" >> ${tmp1}
done
count=$(wc -l ${tmp1} | awk '{ print $1 }')
echo " ${count} readen lines"
#cat ${tmp1}
#exit

# get in tmp2 the same run-ordered lines, just with line numbers as a separated field
#cat ${tmp1} | while read line; do
#	numline=$(echo ${line} | cut -d: -f1)
#	echo "${numline} $(echo ${line} | cut -d' ' -f3-)" >> ${tmp2}
#done
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
#cat ${tmp2} | while read line; do
#	numline=$(echo ${line} | awk '{ print $1 }')
#	fname=$(echo ${line} | awk '{ print $2 }' | sed 's/:$//')
#	address=$(echo ${line} | awk '{ print $3}' | sed -e 's/.*=//' -e 's/,$//')
#	typename=$(echo ${line} | awk '{ print $4 }' | sed 's/[(),]*//g')
#	[ "${typename}" = "" ] && echo "warning: no type in line ${line}" 1>&2
#	prefix=$(echo ${fname} | sed 's/_instance.*$//')
#	nature=1:init
#	[ "$(echo ${fname} | grep init)" = "" ] && nature=2:disp
#	#echo "address='${address}' fname='${fname}' object='${object}' typename='${typename}' numline='${numline}'" >> ${tmp3}
#	#echo -e "${numline}\t ${fname}\t ${address}\t ${typename}\t ${prefix}\t ${rang}" >> ${tmp3}
#	#echo "${numline} ${fname} ${address} ${typename} ${prefix} ${nature}" >> ${tmp3}
#	echo "${numline} ${fname} ${address} ${typename} ${prefix} ${nature}" >> ${tmp3}
#done

export LC_ALL=C

# address, reused_count, function_prefix (should identify the class), line_number
echo "Sorting..."
sortparms="-k7,7n -k3,3 -k5,5 -k1,1n"
cat ${tmp1} | sort ${sortparms} > ${tmp2}

#cat ${tmp3} | sort ${sortparms}
#exit

echo "Apparying..."
line_init=""
count_init count_ok
count_init count_undisposed
count_init count_undisposed_bis
count_init count_warns
str_init list_undisposed
cat ${tmp2} | while read line; do
	nature=$(echo ${line} | awk '{ print $6 }')
	if [ "${line_init}" = "" -a "${nature}" != "1:init" ]; then
		echo "warning: unwaited line: ${line}"
		count_inc count_warns
		continue
	fi
	if [ "${line_init}" = "" ]; then
		addr_init=$(echo ${line} | awk '{ print $3 }')
		app_init=$(echo ${line} | awk '{ print $5 }')
		line_init="${line}"
	else
		addr_dispose=$(echo ${line} | awk '{ print $3 }')
		app_dispose=$(echo ${line} | awk '{ print $5 }')
		line_dispose="${line}"
		if [ "${addr_dispose}" = "${addr_init}" ]; then
			numline_init=$(echo ${line_init} | awk '{ print $1 }')
			numline_dispose=$(echo ${line_dispose} | awk '{ print $1 }')
			type_init=$(echo ${line_init} | awk '{ print $4 }')
			if [ ${display_ok} -eq 1 ]; then
				echo "${type_init} ${addr_init} (${numline_init},${numline_dispose}): OK"
			fi
			count_inc count_ok
			line_init=""
		else
			nature=$(echo ${line_dispose} | awk '{ print $6 }')
			if [ "${nature}" = "1:init" ]; then
				if [ "$(str_grep list_undisposed ${addr_init})" = "" ]; then
					class=$(echo ${line_init} | awk '{ print $4 }')
					num=$(echo ${line_init} | awk '{ print $1 }')
					echo "- undisposed ${class} at ${addr_init} intialized at line ${num}"
					str_add list_undisposed ${addr_init}
					count_inc count_undisposed
				else
					echo "- do not record ${class} already counted"
					count_inc count_undisposed_bis
				fi
			else
				echo "warning: not apparied line: ${line_init}"
				count_inc count_warns
			fi
			addr_init=${addr_dispose}
			app_init=${addr_dispose}
			line_init="${line_dispose}"
		fi
	fi
done

echo ""
echo "Objects are OK    : $(count_display count_ok)"
echo "Undisposed objects: $(count_display count_undisposed) (not re-counted lines: $(count_display count_undisposed_bis))"
echo "Other warnings    : $(count_display count_warns)"

