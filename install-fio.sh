#!/bin/sh
#-----------------------------------------------------------------------------
# Copyright (c) 2006-2014, Fusion-io, Inc.(acquired by SanDisk Corp. 2014)
# Copyright (c) 2014-2015, SanDisk Corp. and/or all its affiliates. All rights reserved.
#  All rights reserved.
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#  * Neither the name of the SanDisk Corp. nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED.
#  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
#  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#-----------------------------------------------------------------------------

DRIVER_DIR="driver"
DRIVER_NAME="iomemory-vsl"
UTILS_DIR="bin"
DOC_DIR="doc"
INSTALLER_NAME="install-fio.sh"

MAN_DIR="usr/local/man/man8"

INSTALL_BASEDIR="/usr/local/fusionio"
UTILS_DESTDIR="$INSTALL_BASEDIR/bin"
UTILS_INSTALLED_FILE="fio_installed_files"

INSTALLED_DRIVER="/boot/modules/${DRIVER_NAME}.ko"
FAKE_ROOT="package_build"
DOPACKAGE=""

COMMENT="Fusion-io VSL and utilities"
DESCRIPTION="This package provides the Fusion-io VSL and associated utilities"

usage() {
    echo
    echo "Usage: install-fio.sh [options]"
    echo "Where install-fio.sh options include:"
    echo "     -h          This usage block"
    echo "     -u          Uninstall the driver and utilities"
    echo "     -p          Creates a FreeBSD binary package"
    echo "     -d [dest]   Overrides the default install location with dest for package creation"
    echo "                 Only Applicable when used with the -p option"
    echo ""
    echo "Default  install-fio.sh"
    echo "     Installs the ioDrive driver and utilities"
    echo
}

# Read some input from the user and decide if it matches a yes or no
# and return the proper result
yes_or_no() {
    while [ 1 = 1 ]; do
        read REPLY
        if  [ "x$REPLY" = "xn" -o "x$REPLY" = "xN" -o "x$REPLY" = "xno" -o "x$REPLY" = "xNo" ]; then
            return 0
        elif [ "x$REPLY" = "xy" -o "x$REPLY" = "xY" -o "x$REPLY" = "xyes" -o "x$REPLY" = "xYes" ]; then
            return 1
        else
            echo "Please enter 'y' or 'n'."
        fi
    done
}


# Do the utilities installation
# $1 is the directory name to install the binary utilities into
#
do_utils_install() {
    utils_install=$(ls $UTILS_DIR)
    target_dir=$1

    rm -f $INSTALL_BASEDIR/$UTILS_INSTALLED_FILE
    mkdir -p $target_dir
    for util in $utils_install
    do
        install -o root -g wheel -m 555 $UTILS_DIR/$util $target_dir
        echo $1/$util >> $INSTALL_BASEDIR/$UTILS_INSTALLED_FILE
    done

    # Make sure we leave the installer so they can use it to
    # uninstall.
    install -o root -g wheel -m 555 $INSTALLER_NAME $INSTALL_BASEDIR/
}

do_utils_uninstall() {
    utils_installed=$(cat $INSTALL_BASEDIR/$UTILS_INSTALLED_FILE)

    for file in $utils_installed
    do
        rm -f $file
    done

    rm -f $INSTALL_BASEDIR/$UTILS_INSTALLED_FILE
    rm -f $INSTALL_BASEDIR/$INSTALLER_NAME
}


# Function for locating and uninstalling utilities
uninstall_utils() {
    if [ ! -e "$INSTALL_BASEDIR/$UTILS_INSTALLED_FILE" ] ; then
        echo "Unable to locate the iodrive utilities."
        return 1
    fi

    echo "Removing ioDrive utilities..."
    do_utils_uninstall
}

check_previous_install() {
    if [ -f $INSTALL_BASEDIR/$UTILS_INSTALLED_FILE ]; then
        echo
        echo "The iodrive utilities have already been installed."
        echo "Please uninstall the driver and utilities first."
        echo
        exit 1
    fi
}

install_util() {
    echo
    echo "Please specify the ioDrive utility install path ($UTILS_DESTDIR):"
    read directory

    # If the user presses enter, set the default
    if [ -z $directory ]; then
        directory=$UTILS_DESTDIR
    fi

    if [ ! -d $directory ]; then
        echo "Making directory $directory"
        mkdir -p $directory

        if [ $? != 0 ]; then
            echo "Failed to make directory, please check permissions and"
            echo "pathname and try again."

            exit 1
        fi
    fi

    echo "Installing ioDrive utilities into $directory"

    do_utils_install $directory
}

uninstall_driver() {
    echo
    echo "Would you like to unload the driver with 'kldunload ${DRIVER_NAME}'? (y/n)"
    yes_or_no

    if [ $? = 1 ]; then
        kldunload ${DRIVER_NAME}

        if [ "$?" -ne 0 ] ; then
            echo "Unable to unload ioDrive driver."
        else
            echo "ioDrive driver has been unloaded."
        fi
    fi

    echo "Removing the driver file"
    rm -f $INSTALLED_DRIVER
    echo "NOTE: Please remove ioDrive related sections from /boot/loader.conf"
}


install_driver() {
    cd $DRIVER_DIR
    echo "Building the driver"
    make
    if [ $? -ne 0 ] ; then
        echo "Driver rebuild failed"
        exit 1
    fi

    if [ -e $INSTALLED_DRIVER ] ; then
        echo "Updating ${DRIVER_NAME} driver..."
    else
        echo "Installing ${DRIVER_NAME} driver..."
    fi

    rm -f $INSTALLED_DRIVER
    make install
    cd ..
}

print_autoload_instructions() {
    echo ""
    echo ""
    echo ""
    echo ""
    echo "To have the driver auto load at boot, add the following"
    echo "lines to /boot/loader.conf:"
    echo ""
    echo "# set auto_attach to 0 to keep ioDrives from attaching the block device at boot"
    echo '# hw.fio.auto_attach="0"'
    echo "${DRIVER_NAME}_load=\"YES\""
    echo ""
}

make_package() {

    basedir="$UTILS_DESTDIR"

    #remove leading / if we have one
    basedir=${basedir#/}/

    cd $DRIVER_DIR
    echo "Building $DRIVER_NAME"
    make clean
    make

    cd ..

    echo "Generating package"
    # remove the build dir if there is one
    rm -rf $FAKE_ROOT

    # make a fake root to build the package from
    mkdir -p $FAKE_ROOT/$basedir
    mkdir -p $FAKE_ROOT/boot/modules
    mkdir -p $FAKE_ROOT/$MAN_DIR

    # copy in every thing we'll need for the binary only package
    cp -r bin/ $FAKE_ROOT/$basedir
    cp $DRIVER_DIR/$DRIVER_NAME.ko $FAKE_ROOT/boot/modules
    cp -r $DOC_DIR/* $FAKE_ROOT/$MAN_DIR

    #remove the old plist file if we have one and create a new pkg-plist file
    rm -f pkg-plist

    for file in $FAKE_ROOT/$basedir*
    do
        echo "$basedir$(basename $file)" >> pkg-plist
    done

    for file in $FAKE_ROOT/$MAN_DIR/*
    do
        echo "$MAN_DIR/$(basename $file)" >> pkg-plist
    done

    echo "boot/modules/$DRIVER_NAME.ko" >> pkg-plist
    echo "@dirrm $basedir" >> pkg-plist

    pkg_create -v -f pkg-plist -s $(pwd)/$FAKE_ROOT -p / -c -"$COMMENT" -d -$"DESCRIPTION" $DRIVER_NAME.tgz

    #clean up
    rm -f pkg-plist
    rm -rf $FAKE_ROOT

    echo "Use sudo pkg_add $DRIVER_NAME.tgz to install the new package."
}

while getopts "uhpd:" opt; do

    case $opt in
        h)
            usage
            exit 0
            ;;
        d)
            UTILS_DESTDIR="$OPTARG"
            ;;
        p)
            DOPACKAGE="TRUE"
            ;;
        u)
            uninstall_utils
            uninstall_driver
            exit 0
            ;;
        \?)
            echo "Invalid Option!"
            usage
            exit 0
    esac
done

if [ "$DOPACKAGE" ]; then
    make_package
    exit 0
fi

check_previous_install

install_util
install_driver
print_autoload_instructions
