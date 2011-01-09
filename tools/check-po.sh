#!/bin/sh
#
# a small script to check the completeness of po/POTFILES.in
#
# should be ran from top_srcdir

PO=po/POTFILES.in
if [ ! -r ${PO} ]; then
	echo "${PO}: file not found" 1>&2
	echo "This script should be ran from top_srcdir." 1>&2
	exit 1
fi

# first, check that all .ui are in po/POTFILE.in
total=0
count=0
errs=0
echo ""
echo "checking that all .ui are in ${PO}..."
for f in $(git ls-files *.ui); do
	if [ "$(grep -xe "\[type:\s*gettext/glade]\s*${f}" ${PO})" = "" ]; then
		echo "	${f} should be added to ${PO}"
		let errs+=1
	fi
	let count+=1
done
echo "pass 1/5: count=${count} error(s)=${errs}"
let total+=${errs}

# second, check that all .ui in PO exist
count=0
errs=0
echo ""
echo "checking that all .ui from ${PO} actually exist..."
for f in $(grep -e '\.ui$' ${PO} | sed 's,\[type:\s*gettext/glade]\s*,,'); do
	if [ ! -r ${f} ]; then
		echo "	${f} should be removed from ${PO}"
		let errs+=1
	fi
	let count+=1
done
echo "pass 2/5: count=${count} error(s)=${errs}"
let total+=${errs}

# third, check that all files which use _( construct are in PO
count=0
errs=0
echo ""
echo "checking that all translatable files are in ${PO}..."
exceptions_pass3="src/test/check-po.sh"
for f in $(git grep '_(' src | cut -d: -f1 | sort -u); do
	if [ "$(echo ${exceptions_pass3} | grep -w ${f})" = "" ]; then
		if [ "$(grep -x ${f} ${PO})" != "${f}" ]; then
			echo "	${f} should be added to ${PO}"
			let errs+=1
		fi
	fi
	let count+=1
done
echo "pass 3/5: count=${count} error(s)=${errs}"
let total+=${errs}

# fourth, check that all files in PO actually use the _( construct
count=0
errs=0
echo ""
echo "checking that all files in ${PO} actually use the '_(' construct..."
for f in $(grep -E '^src/' ${PO} | grep -vE '\.ui$' | grep -vE '\.in$'); do
	grep '_(' ${f} 1>/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "	${f} should be removed from ${PO}"
		let errs+=1
	fi
	let count+=1
done
echo "pass 4/5: count=${count} error(s)=${errs}"
let total+=${errs}

# last, check that all files which include gi18n.h are relevant
count=0
errs=0
echo ""
echo "checking that all files have a good reason to include gi18n.h..."
for f in $(git grep '#include <glib/gi18n.h>' src | cut -d: -f1 | sort -u); do
	grep '_(' ${f} 1>/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "	${f} should not include <glib/gi18n.h>"
		let errs+=1
	fi
	let count+=1
done
echo "pass 5/5: count=${count} error(s)=${errs}"
let total+=${errs}

echo ""
echo "total: ${total} error(s)."
