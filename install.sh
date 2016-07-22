#!/bin/sh

if [ -z "$1" ]; then
  echo "install.sh DEST_PATH"
  echo "ie. install.sh /app"
  exit
fi
dest_dir="$1"

echo "# install dependent tools ..."
#sudo yum install -y http://files.freeswitch.org/freeswitch-release-1-6.noarch.rpm epel-release
#sudo yum install -y gcc-c++ autoconf automake libtool wget python ncurses-devel zlib-devel libjpeg-devel openssl-devel e2fsprogs-devel sqlite-devel libcurl-devel pcre-devel speex-devel ldns-devel libedit-devel libxml2-devel libyuv-devel opus-devel libvpx-devel libvpx2* libdb4* libidn-devel unbound-devel libuuid-devel lua-devel libsndfile-devel yasm-devel
sudo yum install -y gcc-c++ autoconf automake libtool wget python ncurses-devel zlib-devel openssl-devel e2fsprogs-devel sqlite-devel libcurl-devel pcre-devel speex-devel ldns-devel libedit-devel libxml2-devel opus-devel libidn-devel unbound-devel libuuid-devel lua-devel libsndfile-devel

echo "# install dependent tools with rpm packages ..."
tar zxvf deps-pkg.tar.gz
cd deps-pkg
depspkgdir="./"
for file in `ls $depspkgdir`
do
  filepath=$depspkgdir$file
  if [ -f $filepath ]; then
    if [ "$filepath"=~".rpm" ]; then
      sudo rpm -ivh $filepath
    fi
  fi
done
cd ..

# decompression camshare.tar.gz
if [ ! -d "$dest_dir" ]; then
  mkdir $dest_dir
fi
currdir=`pwd`
tempdir="$currdir/tmp"
if [ ! -d "$tempdir" ]; then
  mkdir $tempdir
fi
#tar zxvf $camsharegz_path -C $tempdir/

# define && enter gzdir
tempgzdir="$currdir"

# decompression CamShareServer.tar.gz
camshare_name="CamShareServer"
camsharedir_gz="$tempgzdir/$camshare_name.tar.gz"
camsharedir="$tempdir/$camshare_name"
tar zxvf $camsharedir_gz -C $tempdir
cp -rf $camsharedir $dest_dir

# decompression freeswitch.tar.gz
freeswitch_name="freeswitch"
freeswitchdir_gz="$tempgzdir/$freeswitch_name.tar.gz"
freeswitchdir="$tempdir/$freeswitch_name"
tar zxvf $freeswitchdir_gz -C $tempdir
cp -rf $freeswitchdir $dest_dir

# decompression usr_local_bin.tar.gz
usrlocalbin_dir="/usr/local/bin"
bindir_gz="$tempgzdir/usr_local_bin.tar.gz"
bindir="$tempdir/usr_local_bin"
tar zxvf $bindir_gz -C $tempdir
cp -rf $bindir/* $usrlocalbin_dir/ 

# remove temp dir
rm -rf $tempdir

# add link
camshare_lnpath="/usr/local/$camshare_name"
freeswitch_lnpath="/usr/local/$freeswitch_name"
if [ ! "$dest_dir" = "/usr/local" ]; then
  cd $dest_dir
  currdir=`pwd`
  if [ ! -d "$camshare_lnpath" ]; then
    ln -sfn $currdir/$camshare_name $camshare_lnpath
  fi

  if [ ! -d "$freeswitch_lnpath" ]; then
    ln -sfn $currdir/$freeswitch_name $freeswitch_lnpath
  fi
  cd -
fi

