#!/bin/bash
#
# file:        test-3.sh
# description: Tests for question 3
#
# Akshdeep Rungta, Hongxiang Wang

if [ "x$1" = "x" ]; then
  echo "$0 <dir>"
  echo "Give the mountpoint <dir> where your image has been mounted via FUSE"
  exit 1
fi

MNT=$1
test_n=$2
failedtests=
TMP=/tmp/test3-$$

umask 022

echo Testing

# need to get group ID to compare results properly
#
set $(id | tr '()' ' ')
GROUP=$4

testperm(){
    line=$(ls -l $MNT/$1)
    val=$2
    set x$line
    if [ $1 != x$val ] ; then 
	echo incorrect permissions: $line
	echo $1 should be x$val
	fail=1
    fi
}

checktest(){
    if [ x"$fail" = x1 ] ; then
	echo "  FAILED"
	failedtests="$failedtests $1"
    else
	echo "  SUCCESS"
    fi
    fail=
}

test1(){
  # test 1 - mkdir, ls
  echo Test 1 - mkdir, ls
  mkdir $MNT/dir1/dir2
  mkdir $MNT/dir1/dir2/dir3
  {
    ls  $MNT/dir1 
    ls  $MNT/dir1/dir2 
    ls  $MNT/dir1/dir2/dir4 
    ls  $MNT/dir1/dir4/dir3 
    ls  $MNT/dir1/file.2/dir3 
    ls  $MNT/dir1/file.270
  } > $TMP

diff -u $TMP -  <<EOF 
dir2
file.0
file.2
file.270
dir3
$MNT/dir1/file.270
EOF
  [ $? = 0 ] || fail=1
  checktest 1
}

test2(){
  # test 2 - ls -l
  DATE=$(date +'%Y-%m-%d %H:%M')
  echo Test 2 - ls -l
  ls -l --time-style=long-iso $MNT > $TMP
  ls -l --time-style=long-iso $MNT/dir1 >> $TMP
  ls -l --time-style=long-iso $MNT/dir1/file.2 >> $TMP


  diff -u $TMP - <<EOF
total 4
drwxr-xr-x 1 $USER $GROUP    0 2012-07-13 11:08 dir1
-rwxrwxrwx 1 $USER $GROUP 6644 2012-07-13 11:06 file.7
-rwxrwxrwx 1 $USER $GROUP 1000 2012-07-13 11:04 file.A
total 136
drwxr-xr-x 1 $USER $GROUP      0 $DATE dir2
-rwxrwxrwx 1 $USER $GROUP      0 2012-07-13 11:04 file.0
-rwxrwxrwx 1 $USER $GROUP   2012 2012-07-13 11:04 file.2
-rwxrwxrwx 1 $USER $GROUP 276177 2012-07-13 11:06 file.270
-rwxrwxrwx 1 $USER $GROUP 2012 2012-07-13 11:04 q/dir1/file.2
EOF
  [ $? = 0 ] || fail=1
  checktest 2
}

test3(){
  echo Test 3 - fs_read
  dd if=$MNT/file.A  bs=100 > /tmp/file.A
  cmp /tmp/file.A $MNT/file.A || fail=1
  dd if=$MNT/file.7  bs=1024 > /tmp/file.7.1
  cmp /tmp/file.7.1 $MNT/file.7 || fail=1
  dd if=$MNT/file.7  bs=1500 > /tmp/file.7.2
  cmp /tmp/file.7.2 $MNT/file.7 || fail=1
  dd if=$MNT/file.7  bs=2700 > /tmp/file.7.3
  cmp /tmp/file.7.3 $MNT/file.7 || fail=1
  dd if=$MNT/dir1/file.0  bs=1024 > /tmp/file.0
  cmp /tmp/file.0 $MNT/dir1/file.0 || fail=1
  dd if=$MNT/dir1/file.270  bs=1024 > /tmp/file.270
  cmp /tmp/file.270 $MNT/dir1/file.270 || fail=1
  rm -f /tmp/file.*
  checktest 3
}
test4(){
  echo Test 4 - mkdir, mknod
  DATE=$(date +'%Y-%m-%d %H:%M')
{
  mkdir $MNT/dir1/dir1.1
  mkdir $MNT/dir1/dir1.1/dir1.1.1
  mkdir $MNT/dir1/dir1.2
  mkdir $MNT/dir1/cc/ccc
  mkdir $MNT/dir1/file.0/ccc
  mkdir $MNT/dir1/file.0
  mkdir $MNT/dir1/dir1.1
  mkdir $MNT/cccc/dir2
  mkdir $MNT/file.A/dir2
  mkdir $MNT/fulldir
  mkdir $MNT/fulldir/dir1
  mkdir $MNT/fulldir/dir2
  mkdir $MNT/fulldir/dir3
  mkdir $MNT/fulldir/dir4
  mkdir $MNT/fulldir/dir5
  mkdir $MNT/fulldir/dir6
  mkdir $MNT/fulldir/dir7
  mkdir $MNT/fulldir/dir8
  mkdir $MNT/fulldir/dir9
  mkdir $MNT/fulldir/dir10
  mkdir $MNT/fulldir/dir11
  mkdir $MNT/fulldir/dir12
  mkdir $MNT/fulldir/dir13
  mkdir $MNT/fulldir/dir14
  mkdir $MNT/fulldir/dir15
  mkdir $MNT/fulldir/dir16
  mkdir $MNT/fulldir/dir17
  mkdir $MNT/fulldir/dir18
  mkdir $MNT/fulldir/dir19
  mkdir $MNT/fulldir/dir20
  mkdir $MNT/fulldir/dir21
  mkdir $MNT/fulldir/dir22
  mkdir $MNT/fulldir/dir23
  mkdir $MNT/fulldir/dir24
  mkdir $MNT/fulldir/dir25
  mkdir $MNT/fulldir/dir26
  mkdir $MNT/fulldir/dir27
  mkdir $MNT/fulldir/dir28
  mkdir $MNT/fulldir/dir29
  mkdir $MNT/fulldir/dir30
  mkdir $MNT/fulldir/dir31
  mkdir $MNT/fulldir/dir32
  mkdir $MNT/fulldir/dir33
  touch $MNT/dir1/file.3
  touch $MNT/file.A/file1
  touch $MNT/dirrr/file1
  touch $MNT/dir1/file.3
  touch $MNT/dir1/dir2

} > $TMP
  ls --time-style=long-iso -lR $MNT/dir* >> $TMP
  diff -u $TMP - <<EOF
$MNT/dir1:
total 136
drwxr-xr-x 1 $USER $GROUP      0 $DATE dir1.1
drwxr-xr-x 1 $USER $GROUP      0 $DATE dir1.2
drwxr-xr-x 1 $USER $GROUP      0 $DATE dir2
-rwxrwxrwx 1 $USER $GROUP      0 2012-07-13 11:04 file.0
-rwxrwxrwx 1 $USER $GROUP   2012 2012-07-13 11:04 file.2
-rwxrwxrwx 1 $USER $GROUP 276177 2012-07-13 11:06 file.270
-rw-r--r-- 1 $USER $GROUP      0 $DATE file.3

$MNT/dir1/dir1.1:
total 0
drwxr-xr-x 1 $USER $GROUP 0 $DATE dir1.1.1

$MNT/dir1/dir1.1/dir1.1.1:
total 0

$MNT/dir1/dir1.2:
total 0

$MNT/dir1/dir2:
total 0
drwxr-xr-x 1 $USER $GROUP 0 $DATE dir3

$MNT/dir1/dir2/dir3:
total 0
EOF
  [ $? = 0 ] || fail=1
  checktest 4
}

cksumtest(){
    val=$2
    set $(cat $MNT/$1 | cksum)
    if [ "$val" != "$1 $2" ] ; then
	echo cksum $MNT/$1: $1 $2
	echo should be $val
	fail=1
    fi
}

test5(){
  echo Test 5 - rmdir  
{
  rmdir $MNT/dir9/cc
  rmdir $MNT/file.A/cc
  rmdir $MNT/dir1/cc
  rmdir $MNT/dir1/dir1.1
  rmdir $MNT/dir1/file3
  rmdir $MNT/dir1/dir1.1/dir1.1.1
  ls $MNT/dir1/dir1.1
} > $TMP
  diff -u $TMP - <<EOF
EOF
[ $? = 0 ] || fail=1
   checktest 5
}

test6(){
  echo Test 6 - unlink  
{
  rm $MNT/dir9/cc
  rm $MNT/file.A/cc
  rm $MNT/dir1/cc
  rm $MNT/dir1/dir1.1
  rm $MNT/dir1/file3
  rm $MNT/dir1/file.2
  rm $MNT/dir1/file.270
  rm $MNT/file.A
  rm $MNT/file.7
  ls $MNT
  ls $MNT/dir1
} > $TMP
  diff -u $TMP - <<EOF
dir1
dir2
file2
fulldir
test.l
test.m
test.out
test.s
dir1.1
dir1.2
dir2
file.0
file.3
EOF
[ $? = 0 ] || fail=1
  test "$(./read-img test3.img | tail -2 | wc --words)"==4 || fail=1
  checktest 6
}


test7(){
  # truncate
  echo Test 7 - truncate
  echo foo > $MNT/test.out
  cksumtest test.out "3915528286 4" 
  checktest 7
}

test8(){
  #  write
  echo Test 8 - write
  yes testt | fmt | head --bytes 40 > $MNT/test.s

  yes testt | fmt | head --bytes 7000 > $MNT/test.m
  yes testt | fmt | head --bytes 300000 > $MNT/test.l

  dd if=$MNT/file.A  bs=100 > $MNT/dir2/file.A
  test $(wc –-bytes dir2/file.A) = 1000 || fail = 1
  dd if=$MNT/file.7  bs=1024 > $MNT/dir2/file.7.1
  test $(wc –-bytes dir2/file.7.1) = 1000 || fail = 1
  dd if=$MNT/file.7  bs=1500 > $MNT/dir2/file.7.2
  test $(wc –-bytes dir2/file.7.2) = 1000 || fail = 1
  dd if=$MNT/file.7  bs=2700 > $MNT/dir2/file.7.3
  test $(wc –-bytes dir2/file.7.3) = 1000 || fail = 1
  
  checktest 8
}

test9(){
  # test 9 - chmod utime
  echo Test 9 - chmod
  echo > $MNT/file2
  chmod 777 $MNT/file2
  testperm file2 -rwxrwxrwx
  chmod 321 $MNT/file2 || fail=1
  testperm file2 --wx-w---x
  chmod 444 $MNT/file2 || fail=1
  testperm file2 -r--r--r--
  chmod 740 $MNT/file2 || fail=1
  testperm file2 -rwxr-----
  chmod 754 $MNT/file.A
  test “$(ls -l $MNT/file.A)” = \
  ‘-rwxr-xr-- 1 student student 1000 Jul 13 2012 dir/file.A’ || fail =1
  touch -d ‘Jan 01 2000’ $MNT/file.A
  test “$(ls -l $MNT/file.A)” = \
  ‘-rwxr-xr-- 1 student student 1000 Jan 1 2000 dir/file.A || fail =1
  checktest 9
}


test10(){
  # rename
  echo Test 10 - rename
{
  mv $MNT/filea  $MNT/file321
  mv $MNT/dasd/fileA $MNT/dasd/czxc
  mv $MNT/dir1/filea  $MNT/dir1/file321
  mv $MNT/file.A/filea  $MNT/file.A/file321
  mv $MNT/file.A  $MNT/file.7
  mv $MNT/dir1/file.0 $MNT/file.270
  mv $MNT/dir1/file.0  $MNT/dir1/file.0
  mv $MNT/file.A $MNT/dir2
  mv $MNT/dir1/file.0  $MNT/dir1/dir1.1
  mv $MNT/dir1/file.0  $MNT/dir2/file.0
  mv $MNT/dir1/file.2 $MNT/dir2/file.txt
} > $TMP

  -diff -u $TMP - <<EOF
EOF
  checktest 10
}

test11(){
  # utime
  echo Test 11 - utime
  set x$(ls --full-time $MNT/dir1/file.2)
  oldtime=$7
  sleep 1
  touch $MNT/dir1/file.2
  set x$(ls --full-time $MNT/dir1/file.2)
  newtime=$7
  if [ $oldtime = $newtime ]; then
      echo utime failed: $oldtime $newtime
      fail=1
  fi
  checktest 11
}



if [ "x$test_n" != "x" ]; then
  eval "test$test_n"
else
  test1
  test2
  test3
  test4
  test7
  test8
  test9
  test10
  test11
  test5
  test6
fi

if [ "$failedtests" = "" ] ; then
    echo '*** ALL TESTS PASSED ***'
else
    echo "FAILING TESTS: $failedtests"
fi