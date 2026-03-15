#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
TOOLCHAIN_DYN_LIB_PATH=$(${CROSS_COMPILE}gcc -print-sysroot)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs/bin ${OUTDIR}/rootfs/dev ${OUTDIR}/rootfs/etc ${OUTDIR}/rootfs/home ${OUTDIR}/rootfs/lib ${OUTDIR}/rootfs/lib64 ${OUTDIR}/rootfs/proc ${OUTDIR}/rootfs/sys ${OUTDIR}/rootfs/sbin ${OUTDIR}/rootfs/tmp ${OUTDIR}/rootfs/usr ${OUTDIR}/rootfs/var
mkdir -p ${OUTDIR}/rootfs/usr/lib ${OUTDIR}/rootfs/usr/bin ${OUTDIR}/rootfs/usr/sbin 
mkdir -p ${OUTDIR}/rootfs/var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
cd ${OUTDIR}/rootfs
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
INTERP=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | awk -F"/lib/" '{gsub(/\]/,""); print $2}')
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
LIBS=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk -F'[][]' '{print $2}' | tr '\n' ' ')
# TODO: Add library dependencies to rootfs
for lib in ${INTERP}; do
cp ${TOOLCHAIN_DYN_LIB_PATH}/lib/${lib} ${OUTDIR}/rootfs/lib
done
for lib in ${LIBS}; do
cp ${TOOLCHAIN_DYN_LIB_PATH}/lib64/${lib} ${OUTDIR}/rootfs/lib64
done

# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make writer CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
mkdir -p ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/assignment.txt ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf
cp autorun-qemu.sh ${OUTDIR}/rootfs/home
sed -i 's/..\/conf\/assignment.txt/conf\/assignment.txt/' ${OUTDIR}/rootfs/home/finder-test.sh

# TODO: Chown the root directory
sudo chown root:root ${OUTDIR}/rootfs
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
sudo find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

