#!/bin/sh
#
# file:        test-2.sh
# description: Tests for question 2
#
# Akshdeep Rungta, Hongxiang Wang

if [ x$1 = x-v ] ; then
    verbose=t; shift
fi

cmd=./homework
disk=/tmp/$USER-disk1.img

for f in $cmd; do
  if [ ! -f $f ] ; then
      echo "Unable to access: $f"
      exit
  fi
done
./mktest $disk

yes '0 1 2 3 4 5 6 7' | fmt | head --bytes 5100 > /tmp/test2-file.1
yes '1 2 3' | fmt | head --bytes 20 > /tmp/test2-file.2
yes "3" | fmt | head --bytes 1000 > /tmp/test2-file.3
yes "" | fmt | head --bytes 0 > /tmp/test2-file.0

echo Testing question 1

rm -f /tmp/test2-output
DATE=$(date +'%a %b %d %H:%M:%S %Y')
$cmd -cmdline -image $disk <<EOF > /tmp/test2-output
ls-l file.A
chmod 555 file.A
ls-l file.A
chmod 754 file.A
ls-l file.A
mkdir dir2
mkdir dir1/dir3
ls
rename file.999 file.000
rename file.A file.7
rename file.A ./dir1/file4
rename file.7 file.B
put /tmp/test2-file.1 file.1
put /tmp/test2-file.2 file.2
put /tmp/test2-file.3 file.3
put /tmp/test2-file.0 file.0
ls
show file.2
show file.3
show file.0
put /tmp/test2-file.3 file.0
show file.0
rm file.A
ls
rm dir2
rmdir dir2
rmdir dir1
ls
rm file.999
rmdir dir000
truncate file.B
ls
truncate file.A
get dir1
get file.1 /tmp/get-file.1
quit
EOF
echo >> /tmp/test2-output
[ "$verbose" ] && echo wrote /tmp/test2-output

diff - /tmp/test2-output <<EOF
read/write block size: 1000
cmd> ls-l file.A
/file.A -rwxrwxrwx 1000 1 Fri Jul 13 11:04:40 2012
cmd> chmod 555 file.A
cmd> ls-l file.A
/file.A -r-xr-xr-x 1000 1 $DATE
cmd> chmod 754 file.A
cmd> ls-l file.A
/file.A -rwxr-xr-- 1000 1 $DATE
cmd> mkdir dir2
cmd> mkdir dir1/dir3
cmd> ls
dir1
dir2
file.7
file.A
cmd> rename file.999 file.000
error: No such file or directory
cmd> rename file.A file.7
error: File exists
cmd> rename file.A ./dir1/file4
error: Invalid argument
cmd> rename file.7 file.B
cmd> put /tmp/test2-file.1 file.1
cmd> put /tmp/test2-file.2 file.2
cmd> put /tmp/test2-file.3 file.3
cmd> put /tmp/test2-file.0 file.0
cmd> ls
dir1
dir2
file.0
file.1
file.2
file.3
file.A
file.B
cmd> show file.2
1 2 3 1 2 3 1 2 3 1 cmd> show file.3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
3 3 3 3 3 3 3 cmd> show file.0
cmd> put /tmp/test2-file.3 file.0
error: File exists
cmd> show file.0
cmd> rm file.A
error: Is a directory
cmd> ls
dir1
dir2
file.0
file.1
file.2
file.3
file.A
file.B
cmd> rm dir2
error: Is a directory
cmd> rmdir dir2
cmd> rmdir dir1
error: Directory not empty
cmd> ls
dir1
file.0
file.1
file.2
file.3
file.A
file.B
cmd> rm file.999
error: No such file or directory
cmd> rmdir dir000
error: No such file or directory
cmd> truncate file.B
cmd> ls
dir1
file.0
file.1
file.2
file.3
file.A
file.B
cmd> truncate file.A
cmd> get dir1
cmd> get file.1 /tmp/get-file.1
cmd> quit

EOF

if [ $? != 0 ] ; then
    echo 'Tests may have failed - see above output for details'
else
    echo 'Tests passed - output matches exactly'
fi
[ "$verbose" ] || rm -f /tmp/test2-output