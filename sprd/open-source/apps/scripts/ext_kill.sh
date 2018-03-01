#!/system/bin/sh

term="/dev/pts/* "

if [ "$1" = "-t" ]; then
	setprop ril.t.assert 1
	phone_count=`getprop ro.modem.t.count`
	if [ "$phone_count" = "1" ]; then
		setprop ril.t.sim.power 0
		ssda_mode=`getprop persist.radio.ssda.mode`
		if [ "$ssda_mode" = "svlte" ]; then
			setprop ril.l.sim.power 0
			setprop ril.service.l.enable -1
			setprop ril.lte.cereg.state -1
		fi
	elif [ "$phone_count" = "2" ]; then
		setprop ril.t.sim.power 0
		setprop ril.t.sim.power1 0
	elif [ "$phone_count" = "3" ]; then
		setprop ril.t.sim.power 0
		setprop ril.t.sim.power1 0
		setprop ril.t.sim.power2 0
	fi
	phone=`getprop sys.phone.app`
	kill $phone
elif [ "$1" = "-w" ]; then
	setprop ril.w.assert 1
	phone_count=`getprop ro.modem.w.count`
	if [ "$phone_count" = "1" ]; then
		setprop ril.w.sim.power 0
	elif [ "$phone_count" = "2" ]; then
		setprop ril.w.sim.power 0
		setprop ril.w.sim.power1 0
	elif [ "$phone_count" = "3" ]; then
		setprop ril.w.sim.power 0
		setprop ril.w.sim.power1 0
		setprop ril.w.sim.power2 0
	fi
	phone=`getprop sys.phone.app`
	kill $phone
elif [ "$1" = "-l" ]; then
	setprop ril.l.assert 1
        phone_count=`getprop ro.modem.l.count`
        setprop ril.l.sim.power 0
        if [ "$phone_count" = "2" ]; then
	      setprop ril.l.sim.power1 0
        fi
	ssda_mode=`getprop persist.radio.ssda.mode`
	if [ "$ssda_mode" = "tdd-csfb" -o "$ssda_mode" = "fdd-csfb" -o "$ssda_mode" = "csfb" ]; then
		phone=`getprop sys.phone.app`
		kill $phone
	fi
elif [ "$1" = "-lf" ]; then
        setprop ril.lf.assert 1
        phone_count=`getprop ro.modem.lf.count`
        if [ "$phone_count" = "1" ]; then
              setprop ril.lf.sim.power 0
        elif [ "$phone_count" = "2" ]; then
              setprop ril.lf.sim.power 0
              setprop ril.lf.sim.power1 0
        fi
        phone=`getprop sys.phone.app`
        kill $phone
elif [ "$1" = "-tl" ]; then
        setprop ril.tl.assert 1
        phone_count=`getprop ro.modem.tl.count`
        if [ "$phone_count" = "1" ]; then
              setprop ril.tl.sim.power 0
        elif [ "$phone_count" = "2" ]; then
              setprop ril.tl.sim.power 0
              setprop ril.tl.sim.power1 0
        fi
        phone=`getprop sys.phone.app`
        kill $phone
fi

