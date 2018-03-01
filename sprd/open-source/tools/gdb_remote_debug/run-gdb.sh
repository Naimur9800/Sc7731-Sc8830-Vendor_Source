#!/bin/bash

SCRIPT_NAME=$(basename $0)
. load-config.sh

ADB=adb
GDB=${GDB:-prebuilt/$(uname -s | tr "[[:upper:]]" "[[:lower:]]")-x86/toolchain/arm-linux-androideabi-4.4.x/bin/arm-linux-androideabi-gdb}
[ -f GDB ] || GDB=prebuilt/$(uname -s | tr "[[:upper:]]" "[[:lower:]]")-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-gdb
B2G_BIN=/system/b2g/b2g
GDBINIT=/tmp/b2g.gdbinit.$(whoami)

GONK_OBJDIR=out/target/product/$DEVICE
SYMDIR=$GONK_OBJDIR/symbols

GDBSERVER_PID=$($ADB shell 'toolbox ps gdbserver | (read header; read user pid rest; echo -n $pid)')

GDB_PORT=$((10000 + $(id -u) % 50000))
if [ "$1" = "attach"  -a  -n "$2" ] ; then
   B2G_PID=$2
   if [ -z "$($ADB ls /proc/$B2G_PID)" ] ; then
      echo Error: PID $B2G_PID is invalid
      exit 1;
   fi
   GDB_PORT=$((10000 + ($B2G_PID + $(id -u)) % 50000))
   # cmdline is null separated
   zygote_pid=$($ADB shell ps | grep -P "^root\s+\d+\s+\d+\s+\d+\s+\d+\s+\w+\s+\w+\s+\w+\s+zygote\b" | awk '{print $2}')
   if [ -n "$zygote_pid" ] && $ADB shell ps | grep -P "^\w+\s+$B2G_PID\s+$zygote_pid\s" > /dev/null
   then
      B2G_BIN=/system/bin/app_process
   else
      B2G_BIN=$($ADB shell cat /proc/$B2G_PID/cmdline | tr '\0' '\n' | head -1)
   fi
else
   B2G_PID=$($ADB shell 'toolbox ps b2g | (read header; read user pid rest; echo -n $pid)')
fi

for p in $GDBSERVER_PID ; do
   $ADB shell cat /proc/$p/cmdline | grep -q :$GDB_PORT && ( \
      echo ..killing gdbserver pid $p
      $ADB shell kill $p
   ) || echo ..ignoring gdbserver pid $p

done

$ADB forward tcp:$GDB_PORT tcp:$GDB_PORT

if [ "$1" = "attach" ]; then
   if [ -z "$B2G_PID" ]; then
      echo Error: No PID to attach to. B2G not running?
      exit 1
   fi

   $ADB shell gdbserver :$GDB_PORT --attach $B2G_PID &
else
   if [ -n "$1" ]; then
      B2G_BIN=$1
      shift
   fi
   [ -n "$MOZ_PROFILER_STARTUP" ] && GDBSERVER_ENV="$GDBSERVER_ENV MOZ_PROFILER_STARTUP=$MOZ_PROFILER_STARTUP "
   [ -n "$MOZ_DEBUG_CHILD_PROCESS" ] && GDBSERVER_ENV="$GDBSERVER_ENV MOZ_DEBUG_CHILD_PROCESS=$MOZ_DEBUG_CHILD_PROCESS "
   [ -n "$MOZ_DEBUG_APP_PROCESS" ] && GDBSERVER_ENV="$GDBSERVER_ENV MOZ_DEBUG_APP_PROCESS='$MOZ_DEBUG_APP_PROCESS' "
   [ -n "$MOZ_IPC_MESSAGE_LOG" ]     && GDBSERVER_ENV="$GDBSERVER_ENV MOZ_IPC_MESSAGE_LOG=$MOZ_IPC_MESSAGE_LOG "

   [ -n "$B2G_PID" ] && $ADB shell kill $B2G_PID
   [ "$B2G_BIN" = "/system/b2g/b2g" ] && $ADB shell stop b2g

   if [ "$($ADB shell "if [ -f /system/b2g/libdmd.so ]; then echo 1; fi")" != "" ]; then
     echo ""
     echo "Using DMD."
     echo ""
     dmd="1"
     ld_preload_extra="/system/b2g/libdmd.so"
  fi

   $ADB shell LD_LIBRARY_PATH=/system/b2g TMPDIR=/data/local/tmp $GDBSERVER_ENV gdbserver --multi :$GDB_PORT $B2G_BIN $@ &
fi

sleep 1
echo "set solib-absolute-prefix $SYMDIR" > $GDBINIT
echo "handle SIGPIPE nostop" >> $GDBINIT
echo "set solib-search-path $GECKO_OBJDIR/dist/bin:$SYMDIR/system/lib:$SYMDIR/system/lib/hw:$SYMDIR/system/lib/egl:$SYMDIR/system/bin:$GONK_OBJDIR/system/lib:$GONK_OBJDIR/system/lib/egl:$GONK_OBJDIR/system/lib/hw:$GONK_OBJDIR/system/lib/soundfx:$GONK_OBJDIR/system/vendor/lib:$GONK_OBJDIR/system/vendor/lib/hw:$GONK_OBJDIR/system/vendor/lib/egl" >> $GDBINIT
echo "target extended-remote :$GDB_PORT" >> $GDBINIT

PROG=$GECKO_OBJDIR/dist/bin/$(basename $B2G_BIN)
[ -f $PROG ] || PROG=${SYMDIR}${B2G_BIN}

if [ "$SCRIPT_NAME" == "run-ddd.sh" ]; then
    echo "ddd --debugger \"$GDB -x $GDBINIT\" $PROG"
    ddd --debugger "$GDB -x $GDBINIT" $PROG
else
    echo $GDB -x $GDBINIT $PROG
    $GDB -x $GDBINIT $PROG
fi
