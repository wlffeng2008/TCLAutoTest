#!/vendor/bin/sh
#chmod 777 /var
#chmod 777 /tmp
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/vendor/tvos/libBionic:/vendor/tvos/libBionic/ffmpeg
export F1_OS_SYSTEM=/vendor/bin/sh
export F1_CONFIG=/tclconfig/json_sys.ini
export F1_TVSDK=OFF

if [ "`getprop ro.build.type`" == "userdebug" ]; then
/vendor/tvos/bin/logcat -G 50M
fi

/vendor/tvos/bin/log_out -l -u -i 'fpp' &

echo "run tcl mw process start"


# create EPG database in RAM
#mkdir /userdata/epg 0777 root system
#mount -t tmpfs -o size=32m,mode=0777 tmpfs /userdata/epg
#touch /userdata/epg/EpgData.db
#chmod 0666 /userdata/epg/EpgData.db

/vendor/tvos/bin/sitatvservice &

#run appmanager
/vendor/tvos/bin/appmanager --app-path=/product/am_apps:/product/am_apps/tplayer --log=logcat &

/vendor/tvos/bin/ai_pq_service &


#echo fsnum 8 > /proc/msp/vdec_ctrl

echo "run tcl mw process end"

# ifdef VENDOR_EDIT
# wenjie2.yu@BT, 2020/08/26, exe btmonitor
/vendor/bin/btmonitor &
# endif /* VENDOR_EDIT */

logcat -G 50m
logcat -v threadtime >/data/debug.txt &

while true
do
    sleep 3600
done
