#!/bin/sh
#
# A script to check that we have as many instance_finalize that instance_init
# in our debug traces.
#
# $1 = input file (debug log)
#
#real	6m1.314s
#user	0m27.317s
#sys	1m28.583s

function usage 
{
	echo "Usage: $0 <logfile> [display_ok=true|false] - default to 'false'" 1>&2
}

if [ "$1" = "" ]; then
	usage
	exit 1
fi

if [ "${2}" = "" ]; then
	display_ok=0
elif [ "${2}" = "display_ok=false" ]; then
	display_ok=0
elif [ "${2}" = "display_ok=true" ]; then
	display_ok=1
else
	usage
	exit 1
fi

check_for="finalize"

tmp1=/tmp/${0##*/}.$$.1
tmp2=/tmp/${0##*/}.$$.2
tmp3=/tmp/${0##*/}.$$.3

\rm -f /tmp/${0##*/}.*
echo ""

function getftmp
{
	echo /tmp/${0##*/}.$$.${1}
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
	typeset fused=/tmp/${0##*/}.$$.reused
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
# it is kept in run order (input line number order)
# note that instance_init on a base class has the base class name
# while instance_finalize on this same base class has the derived class name
echo -n "Extracting and formatting relevant lines from ${1}"
count100=0
count500=0
total=0
grep -En "instance_init|instance_${check_for}" ${1} | grep -vE 'quitting main window|parent=|children=|tree=|deleted=' | while read line; do
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
	fn_name=$(echo ${line} | awk '{ print $3 }' | sed 's/:$//')
	obj_address=$(echo ${line} | awk '{ print $4 }' | sed -e 's/.*=//' -e 's/,$//')
	class=$(echo ${line} | awk '{ print $5 }' | sed 's/[(),]*//g')
	[ "${class}" = "" ] && echo "warning: no class found in line '${line}'" 1>&2
	prefix=$(echo ${fn_name} | sed 's/_instance.*$//')
	nature="1:init"
	[ "$(echo ${fn_name} | grep instance_init)" = "" ] && nature="2:final"
	reused=$(is_reused ${obj_address}-${prefix}-${nature})
	echo "${numline} ${fn_name} ${obj_address} ${class} ${prefix} ${nature} ${reused}" >> ${tmp1}
done
count=$(wc -l ${tmp1} | awk '{ print $1 }')
echo " ${count} readen lines"
#cat ${tmp1}
#exit

# sort on: reused_count, object_address, class_name, line_number
echo "Sorting..."
sortparms="-k7,7n -k3,3 -k5,5 -k1,1n"
cat ${tmp1} | LC_ALL=C sort ${sortparms} > ${tmp2}

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
	if [ "${nature}" = "1:init" ]; then
		if [ "${line_init}" = "" ]; then
			# set line init
			obj_address_init=$(echo ${line} | awk '{ print $3 }')
			fn_prefix_init=$(echo ${line} | awk '{ print $5 }')
			line_init="${line}"
		else
			# init line being readen, but previous was also an init line
			# say previous was an error
			# and restart with init line being readen
			class=$(echo ${line_init} | awk '{ print $4 }')
			if [ "$(str_grep list_undisposed ${obj_address_init})" = "" ]; then
				num=$(echo ${line_init} | awk '{ print $1 }')
				echo "- unfinalized ${class} at ${obj_address_init} intialized at line ${num}"
				str_add list_undisposed ${obj_address_init}
				count_inc count_undisposed
			else
				echo "- do not record ${class} at ${obj_address_init} already counted"
				count_inc count_undisposed_bis
			fi
			#
			obj_address_init=$(echo ${line} | awk '{ print $3 }')
			fn_prefix_init=$(echo ${line} | awk '{ print $5 }')
			line_init="${line}"
		fi
	else
		if [ "${line_init}" = "" ]; then
			# dispose line being readen but previous was also a dispose line
			# just signals the dispose line is an error
			echo "warning: unwaited line: ${line}"
			count_inc count_warns
		else
			# we have an init line, and are reading a dispose line
			# test if they are for the same object
			class=$(echo ${line} | awk '{ print $4 }')
			obj_address_dispose=$(echo ${line} | awk '{ print $3 }')
			fn_prefix_dispose=$(echo ${line} | awk '{ print $5 }')
			line_dispose="${line}"
			if [ "${obj_address_dispose}" = "${obj_address_init}" ]; then
				numline_init=$(echo ${line_init} | awk '{ print $1 }')
				numline_dispose=$(echo ${line_dispose} | awk '{ print $1 }')
				type_init=$(echo ${line_init} | awk '{ print $4 }')
				if [ ${display_ok} -eq 1 ]; then
					echo "${type_init} ${addr_init} (${numline_init},${numline_dispose}): OK"
				fi
				count_inc count_ok
				line_init=""
			# if they are not for the same object, the two are errors
			else
				class=$(echo ${line_init} | awk '{ print $4 }')
				if [ "$(str_grep list_undisposed ${obj_address_init})" = "" ]; then
					num=$(echo ${line_init} | awk '{ print $1 }')
					echo "- unfinalized ${class} at ${obj_address_init} intialized at line ${num}"
					str_add list_undisposed ${obj_address_init}
					count_inc count_undisposed
				else
					echo "- do not record ${class} at ${obj_address_init} already counted"
					count_inc count_undisposed_bis
				fi
				#
				echo "warning: unwaited line: ${line}"
				count_inc count_warns
				#
				line_init=""
			fi
		fi
	fi
	echo "${line_init}" > /tmp/${0##*/}.$$.line_init
done
# does not work because shell variables do not go out of a while loop in bash
#if [ "${line_init}" != "" ]; then
#	class=$(echo ${line_init} | awk '{ print $4 }')
#	if [ "$(str_grep list_undisposed ${obj_address_init})" = "" ]; then
#		num=$(echo ${line_init} | awk '{ print $1 }')
#		echo "- unfinalized ${class} at ${obj_address_init} intialized at line ${num}"
#		str_add list_undisposed ${obj_address_init}
#		count_inc count_undisposed
#	else
#		echo "- do not record ${class} at ${obj_address_init} already counted"
#		count_inc count_undisposed_bis
#	fi
#fi
line_init="$(cat /tmp/${0##*/}.$$.line_init)"
if [ "${line_init}" != "" ]; then
	obj_address_init=$(echo "${line_init}" | awk '{ print $3 }')
	class=$(echo "${line_init}" | awk '{ print $4 }')
	if [ "$(str_grep list_undisposed ${obj_address_init})" = "" ]; then
		num=$(echo "${line_init}" | awk '{ print $1 }')
		echo "- unfinalized ${class} at ${obj_address_init} intialized at line ${num}"
		str_add list_undisposed ${obj_address_init}
		count_inc count_undisposed
	else
		echo "- do not record ${class} at ${obj_address_init} already counted"
		count_inc count_undisposed_bis
	fi
fi

echo ""
echo "Objects are OK     : $(count_display count_ok)"
echo "Unfinalized objects: $(count_display count_undisposed) (not re-counted lines: $(count_display count_undisposed_bis))"
echo "Other warnings     : $(count_display count_warns)"
